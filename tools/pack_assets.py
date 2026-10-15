#!/usr/bin/env python3
"""Pack prepared MH2 assets into deterministic images.pak and audio.pak."""

import argparse
from pathlib import Path
import re
import struct
import tempfile
import zlib

MAGIC = b"MHASSETS"
MAX_ENTRIES = 256
MAX_BYTES = 100_000_000


def safe_path(path):
    if not isinstance(path, str) or not path or len(path) > 240:
        return False
    reserved = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)),
                *(f"lpt{i}" for i in range(1, 10))}
    return all(re.fullmatch(r"[A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)*", part)
               and part.split(".", 1)[0].lower() not in reserved for part in path.split("/"))


def parse_manifest(data):
    if len(data) > 2_000_000:
        raise ValueError("Oversized manifest")
    lines = iter(data.decode("ascii").splitlines())

    def row(label, count):
        fields = next(lines, "").split()
        if len(fields) != count or fields[0] != label:
            raise ValueError(f"Expected {label} row with {count} fields")
        return fields

    def number(value, low, high):
        if not re.fullmatch(r"[0-9]+", value):
            raise ValueError("Expected unsigned integer")
        result = int(value)
        if not low <= result <= high:
            raise ValueError("Integer outside bounds")
        return result

    if next(lines, "") != "MH2ASSETS":
        raise ValueError("Expected MH2ASSETS header")
    manifest = {"images": [], "audio": []}
    ids = set()
    for group, label, prefix, count in (("images", "image", "gfx/", 12),
                                        ("audio", "audio", "sfx/", 6)):
        total = number(row(group, 2)[1], 1 if group == "images" else 0, 256)
        previous = ""
        for _ in range(total):
            fields = row(label, count)
            asset_id = fields[1]
            if not safe_path(asset_id) or not asset_id.startswith(prefix):
                raise ValueError("Unsafe or wrong-kind asset ID")
            if asset_id in ids or asset_id <= previous:
                raise ValueError("Duplicate or unordered asset ID")
            ids.add(asset_id)
            previous = asset_id
            if group == "images":
                kind = fields[2]
                width, height = (number(value, 1, 32768) for value in fields[3:5])
                if width * height > 25_000_000:
                    raise ValueError("Image exceeds pixel allocation limit")
                png_bytes = number(fields[5], 1, MAX_BYTES)
                indices_bytes, palette_bytes = (number(value, 0, MAX_BYTES) for value in fields[6:8])
                if kind == "indexed":
                    if indices_bytes != width * height or not 4 <= palette_bytes <= 1024 or palette_bytes % 4:
                        raise ValueError("Invalid index/palette lengths")
                elif kind != "truecolor" or indices_bytes or palette_bytes:
                    raise ValueError("Invalid image kind/index metadata")
                columns, rows, cell_width, cell_height = (number(value, 0, 32768) for value in fields[8:12])
                if any((columns, rows, cell_width, cell_height)):
                    if (not all((columns, rows, cell_width, cell_height)) or columns > width or rows > height
                            or cell_width != width // columns or cell_height != height // rows):
                        raise ValueError("Sprite grid differs from image dimensions")
                    grid = {"columns": columns, "rows": rows,
                            "cell_width": cell_width, "cell_height": cell_height}
                else:
                    grid = None
                manifest[group].append({"id": asset_id, "kind": kind, "width": width, "height": height,
                                        "png": {"path": f"images/{asset_id}.png", "bytes": png_bytes},
                                        "indices": {"path": f"indexed/{asset_id}.indices", "bytes": indices_bytes} if indices_bytes else None,
                                        "palette": {"path": f"indexed/{asset_id}.palette", "bytes": palette_bytes} if palette_bytes else None,
                                        "sprite_grid": grid})
            else:
                manifest[group].append({"id": asset_id,
                                        "file": {"path": f"audio/{asset_id}.mp3", "bytes": number(fields[2], 1, MAX_BYTES)},
                                        "metadata": {"sample_rate": number(fields[3], 1, 384000),
                                                     "channels": number(fields[4], 1, 8),
                                                     "decoded_frames": number(fields[5], 1, 1_000_000_000)}})
    if next(lines, None) is not None:
        raise ValueError("Unexpected trailing manifest data")
    return manifest


def read_manifest(path):
    return parse_manifest(path.read_bytes())


def write_changed(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file() and path.read_bytes() == data:
        return False
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + ".", delete=False) as stream:
        temporary = Path(stream.name)
        try:
            stream.write(data)
            stream.flush()
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        temporary.chmod(0o644)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    return True


def collect_files(manifest_path):
    manifest_path = manifest_path.resolve(strict=True)
    root = manifest_path.parent
    manifest_bytes = manifest_path.read_bytes()
    manifest = parse_manifest(manifest_bytes)
    groups = {"images": {"manifest.txt": manifest_bytes}, "audio": {"manifest.txt": manifest_bytes}}
    for group, records in manifest.items():
        for record in records:
            if group == "images":
                files = [record["png"]]
                if record["kind"] == "indexed":
                    files += [record["indices"], record["palette"]]
            else:
                files = [record["file"]]
            for descriptor in files:
                relative = descriptor["path"]
                path = (root / relative).resolve(strict=True)
                if not path.is_relative_to(root) or not path.is_file():
                    raise ValueError(f"Runtime path escapes root: {relative}")
                data = path.read_bytes()
                if len(data) != descriptor["bytes"]:
                    raise ValueError(f"Byte count mismatch: {relative}")
                groups[group][relative] = data
    return {name: sorted(files.items()) for name, files in groups.items()}


def pack_files(files):
    if not 1 <= len(files) <= MAX_ENTRIES:
        raise ValueError("Package entry count outside limits")
    index_size = sum(22 + len(name.encode("ascii")) for name, _ in files)
    offset = 16 + index_size
    index, payload = bytearray(), bytearray()
    previous = ""
    for name, data in files:
        if not safe_path(name) or (previous and previous >= name):
            raise ValueError("Asset paths must be safe, sorted and unique")
        previous = name
        encoded = name.encode("ascii")
        index.extend(struct.pack("<HQQI", len(encoded), offset, len(data), zlib.crc32(data)))
        index.extend(encoded)
        payload.extend(data)
        offset += len(data)
    if offset > MAX_BYTES:
        raise ValueError("Package byte count outside limits")
    return MAGIC + struct.pack("<II", len(files), index_size) + index + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    try:
        groups = collect_files(args.manifest)
        for name, files in groups.items():
            data = pack_files(files)
            path = args.output_dir / f"{name}.pak"
            changed = write_changed(path, data)
            print(f"{path}: {len(files)} entries, {len(data)} bytes, {'written' if changed else 'unchanged'}")
    except (ValueError, KeyError, OSError, TypeError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()

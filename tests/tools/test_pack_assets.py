#!/usr/bin/env python3
"""Deterministic package layout and rejection of damaged or escaping inputs."""

import os
from pathlib import Path
import stat
import struct
import sys
import tempfile
import unittest
import zlib

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY / "tools"))
from pack_assets import collect_files, pack_files, parse_manifest, read_manifest, write_changed


class PackAssetsTest(unittest.TestCase):
    def test_exact_package_layout_and_incremental_write(self):
        files = [("a.png", b"image"), ("b.mp3", b"audio")]
        # Independent format oracle: 16-byte header, two 27-byte entries.
        expected = (b"MHASSETS" + struct.pack("<II", 2, 54)
                    + struct.pack("<HQQI", 5, 70, 5, zlib.crc32(b"image")) + b"a.png"
                    + struct.pack("<HQQI", 5, 75, 5, zlib.crc32(b"audio")) + b"b.mp3"
                    + b"imageaudio")
        self.assertEqual(pack_files(files), expected)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "images.pak"
            self.assertTrue(write_changed(path, expected))
            before = path.stat()
            if os.name == "posix":
                self.assertEqual(stat.S_IMODE(before.st_mode), 0o644)
            self.assertFalse(write_changed(path, expected))
            self.assertEqual((path.stat().st_ino, path.stat().st_mtime_ns),
                             (before.st_ino, before.st_mtime_ns))

    def test_unsafe_duplicate_unsorted_and_excessive_entries_are_rejected(self):
        for name in ("../x", "/x", "x//y", "x/", "a\\b", "a:b", "NUL.txt", "com1.mp3", "x\n"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                pack_files([(name, b"x")])
        for files in ([], [("a", b"x"), ("a", b"y")], [("b", b"x"), ("a", b"y")],
                      [(f"a{i:03}", b"") for i in range(257)]):
            with self.assertRaises(ValueError):
                pack_files(files)

    def test_text_manifest_keeps_only_runtime_fields_and_rejects_invalid_rows(self):
        source = (b"MH2ASSETS\nimages 1\n"
                  b"image gfx/title truecolor 7 5 5 0 0 2 2 3 2\n"
                  b"audio 1\naudio sfx/shot 8 22050 2 400\n")
        manifest = parse_manifest(source)
        self.assertEqual(manifest["images"][0]["sprite_grid"],
                         {"columns": 2, "rows": 2, "cell_width": 3, "cell_height": 2})
        self.assertEqual(manifest["audio"][0]["metadata"],
                         {"sample_rate": 22050, "channels": 2, "decoded_frames": 400})
        for invalid in (source.replace(b"MH2ASSETS", b"MH2ASSETS 1"), source + b"extra\n",
                        source.replace(b"images 1", b"images 2"),
                        source.replace(b"gfx/title", b"../title"),
                        source.replace(b"truecolor 7", b"truecolor -7"),
                        source.replace(b"5 0 0 2 2 3 2", b"5 1 0 2 2 3 2"),
                        source.replace(b"2 2 3 2", b"2 0 3 2"),
                        source.replace(b"22050 2 400", b"22050 0 400")):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                parse_manifest(invalid)

        duplicate = source.replace(b"images 1", b"images 2").replace(
            b"audio 1", b"image gfx/title truecolor 7 5 5 0 0 2 2 3 2\naudio 1")
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            parse_manifest(duplicate)

    def test_manifest_collection_rejects_tamper_and_symlink_escape(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            image = root / "images/gfx/title.png"
            image.parent.mkdir(parents=True)
            image.write_bytes(b"image")
            path = root / "manifest.txt"
            path.write_bytes(b"MH2ASSETS\nimages 1\nimage gfx/title truecolor 1 1 5 0 0 0 0 0 0\naudio 0\n")
            groups = collect_files(path)
            self.assertEqual(dict(groups["images"])["images/gfx/title.png"], b"image")
            self.assertEqual(dict(groups["images"])["manifest.txt"], dict(groups["audio"])["manifest.txt"])
            self.assertEqual(dict(groups["audio"]), {"manifest.txt": path.read_bytes()})
            self.assertEqual(read_manifest(path)["images"][0]["png"]["bytes"], 5)
            image.write_bytes(b"imag")
            with self.assertRaisesRegex(ValueError, "Byte count"):
                collect_files(path)
            image.unlink()
            outside = root.parent / (root.name + "-outside.png")
            try:
                outside.write_bytes(b"image")
                image.symlink_to(outside)
                with self.assertRaisesRegex(ValueError, "escapes"):
                    collect_files(path)
            finally:
                outside.unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()

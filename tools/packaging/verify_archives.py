#!/usr/bin/env python3
"""Check the expected release ZIP set before upload or publication."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import zipfile


RELEASE_ARCHIVES = {
    ("Linux", "x86_64"): (
        "moorhuhn2-Linux-x86_64.zip",
        "moorhuhn2-Linux-x86_64-bundled.zip",
    ),
    ("Windows", "x86_64"): ("moorhuhn2-Windows-x86_64-bundled.zip",),
    ("macOS", "arm64"): ("moorhuhn2-macOS-arm64-bundled.zip",),
    ("macOS", "x86_64"): ("moorhuhn2-macOS-x86_64-bundled.zip",),
}


def verify_archives(directory: Path, platform: str | None = None,
                    arch: str | None = None, output_dir: Path | None = None) -> list[Path]:
    """Validate every expected member before copying any archive to publication staging."""
    names = [name for (system, machine), archives in RELEASE_ARCHIVES.items()
             if (platform is None or platform == system) and (arch is None or arch == machine)
             for name in archives]
    if not names:
        raise ValueError(f"No release archives defined for {platform}/{arch}")
    archives = [directory / name for name in names]
    for path in archives:
        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError(f"Missing or empty release archive: {path}")
        with zipfile.ZipFile(path) as archive:
            if not archive.infolist():
                raise ValueError(f"Release archive has no members: {path}")
            bad_member = archive.testzip()
            if bad_member is not None:
                raise ValueError(f"Corrupt release member {bad_member!r} in {path}")
    if output_dir is not None:
        output_dir.mkdir(parents=True, exist_ok=True)
        for path in archives:
            shutil.copy2(path, output_dir / path.name)
    return archives


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, default=Path("dist"))
    parser.add_argument("--platform", choices=("Linux", "Windows", "macOS"))
    parser.add_argument("--arch", choices=("x86_64", "arm64"))
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    try:
        paths = verify_archives(args.directory, args.platform, args.arch, args.output_dir)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, f"Release verification failed: {error}\n")
    for path in paths:
        print(f"Verified {path}")


if __name__ == "__main__":
    main()

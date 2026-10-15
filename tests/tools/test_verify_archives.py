#!/usr/bin/env python3
"""Release verification must reject incomplete or damaged publication sets."""

from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.packaging.verify_archives import RELEASE_ARCHIVES, verify_archives


class ArchiveVerificationTests(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.dist = self.root / "dist"
        self.dist.mkdir()
        self.output = self.root / "verified"
        for names in RELEASE_ARCHIVES.values():
            for name in names:
                with zipfile.ZipFile(self.dist / name, "w") as archive:
                    archive.writestr("test.txt", b"release payload")

    def test_complete_set_is_verified_and_copied(self) -> None:
        (self.dist / "unrelated.zip").write_bytes(b"ignored")
        paths = verify_archives(self.dist, output_dir=self.output)
        self.assertEqual(len(paths), 5)
        self.assertEqual({p.name for p in paths}, {p.name for p in self.output.iterdir()})
        for path in paths:
            self.assertEqual(path.read_bytes(), (self.output / path.name).read_bytes())

    def test_platform_builds_verify_only_their_expected_archives(self) -> None:
        for (platform, arch), names in RELEASE_ARCHIVES.items():
            with self.subTest(platform=platform, arch=arch):
                self.assertEqual([p.name for p in verify_archives(self.dist, platform, arch)], list(names))
        with self.assertRaises(ValueError):
            verify_archives(self.dist, "Windows", "arm64")

    def test_missing_empty_and_corrupt_archives_block_staging(self) -> None:
        last = self.dist / "moorhuhn2-macOS-x86_64-bundled.zip"
        original = last.read_bytes()
        # Damage member data without changing the ZIP directory or its recorded CRC.
        damaged = original.replace(b"release payload", b"damaged payload")
        self.assertNotEqual(damaged, original)
        with zipfile.ZipFile(self.root / "empty.zip", "w"):
            pass
        for data in (None, b"", b"not a zip", damaged, (self.root / "empty.zip").read_bytes()):
            with self.subTest(data=data):
                if data is None:
                    last.unlink()
                else:
                    last.write_bytes(data)
                with self.assertRaises((ValueError, zipfile.BadZipFile)):
                    verify_archives(self.dist, output_dir=self.output)
                self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()

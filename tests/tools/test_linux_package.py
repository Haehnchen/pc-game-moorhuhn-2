#!/usr/bin/env python3
"""Linux ELF runtime, ldd parsing and packed asset checks."""

import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch


REPOSITORY = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("linux_package", REPOSITORY / "tools/packaging/linux_package.py")
package_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(package_tool)


def write_assets(root):
    root.mkdir(parents=True, exist_ok=True)
    image, audio = b"small image", b"small audio"
    for name, data in (("images/gfx/title.png", image), ("audio/sfx/start.mp3", audio)):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    (root / "manifest.txt").write_text(
        f"MH2ASSETS\nimages 1\nimage gfx/title truecolor 1 1 {len(image)} 0 0 0 0 0 0\n"
        f"audio 1\naudio sfx/start {len(audio)} 22050 1 441\n")


class LddTest(unittest.TestCase):
    def test_ldd_parses_full_closure_and_absolute_libraries(self):
        result = package_tool.parse_ldd(
            "linux-vdso.so.1 (0x1234)\n"
            "libSDL3.so.0 => /sdk/lib/libSDL3.so.0 (0xabcd)\n"
            "libc.so.6 => /lib/libc.so.6 (0x1234)\n"
            "/lib64/ld-linux-x86-64.so.2 (0x1234)\n"
            "/opt/libcustom.so (0x1234)\n"
        )
        self.assertEqual(result["libSDL3.so.0"], Path("/sdk/lib/libSDL3.so.0"))
        self.assertEqual(result["libcustom.so"], Path("/opt/libcustom.so"))
        self.assertIn("ld-linux-x86-64.so.2", result)
        self.assertNotIn("linux-vdso.so.1", result)

    def test_ldd_rejects_missing_conflicting_and_unrecognized_dependencies(self):
        for output in (
            "libSDL3.so.0 => not found\n",
            "libsame.so => /one/libsame.so (0x1)\nlibsame.so => /two/libsame.so (0x2)\n",
            "unexpected dependency output\n",
        ):
            with self.subTest(output=output), self.assertRaises(ValueError):
                package_tool.parse_ldd(output)

    def test_host_glibc_filter_keeps_non_glibc_runtimes(self):
        for name in ("libc.so.6", "libm.so.6", "libmvec.so.1", "libnss_files.so.2",
                     "libresolv.so.2", "ld-linux-aarch64.so.1", "ld64.so.1"):
            self.assertIsNotNone(package_tool.HOST_GLIBC.fullmatch(name), name)
        for name in ("libstdc++.so.6", "libgcc_s.so.1", "libSDL3.so.0", "libdecor-0.so.0"):
            self.assertIsNone(package_tool.HOST_GLIBC.fullmatch(name), name)


class RuntimeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="moorhuhn2-runtime-test-")
        self.addCleanup(self.temporary.cleanup)
        self.stage = Path(self.temporary.name)
        for name in {package_tool.EXECUTABLE} | package_tool.BUNDLED_LIBRARIES:
            (self.stage / name).write_bytes(b"\x7fELF\0runtime")

    def elf_output(self, arguments, **unused):
        if arguments[0] == "strip":
            return ""
        if arguments[0] == "ldd":
            lines = [f" {name} => {self.stage / name} (0x01)" for name in sorted(package_tool.BUNDLED_LIBRARIES)]
            lines += [" libc.so.6 => /usr/lib/libc.so.6 (0x02)", " /lib64/ld-linux-x86-64.so.2 (0x03)"]
            return "\n".join(lines)
        flag, name = arguments[1], Path(arguments[-1]).name
        if flag == "-h":
            return "Class: ELF64\nMachine: Advanced Micro Devices X86-64"
        if flag == "-d":
            return "" if name in package_tool.COMPILER_LIBRARIES else " (RUNPATH) Library runpath: [$ORIGIN]"
        if flag == "-S":
            return ".dynsym"
        if flag == "--version-info":
            version = "2.40" if name == "libstdc++.so.6" else "2.17"
            return ("Version definition section '.gnu.version_d' contains 1 entry:\n"
                    "  Name: GLIBC_9.99\nVersion needs section '.gnu.version_r' contains 1 entry:\n"
                    f"  Name: GLIBC_{version} Flags: none Version: 2")
        raise AssertionError(arguments)

    def test_runtime_accepts_bundled_libraries_and_counts_only_needed_glibc_versions(self):
        with patch.object(package_tool, "run", side_effect=self.elf_output):
            package_tool.check_runtime(self.stage, set(), "strip")
        versions = self.elf_output(["readelf", "--version-info", "libstdc++.so.6"])
        self.assertEqual(package_tool.glibc_requirements(versions), {"2.40"})
        with self.assertRaisesRegex(ValueError, "Unrecognized GLIBC"):
            package_tool.glibc_requirements("Version needs section '.gnu.version_r' contains 1 entry:\n Name: GLIBC_PRIVATE")

    def test_runtime_rejects_host_resolution_missing_library_rpath_and_debug_symbols(self):
        def broken(arguments, **unused):
            output = self.elf_output(arguments)
            if mode == "host" and arguments[0] == "ldd":
                return output.replace(str(self.stage / "libSDL3.so.0"), "/usr/lib/libSDL3.so.0")
            if mode == "missing" and arguments[0] == "ldd":
                return output + "\n libforeign.so => not found"
            if mode == "rpath" and arguments[0] == "readelf" and arguments[1] == "-d":
                return output.replace("$ORIGIN", "/developer/build")
            if mode == "symbols" and arguments[0] == "readelf" and arguments[1] == "-S":
                return ".symtab"
            if mode == "dependency" and arguments[0] == "readelf" and arguments[1] == "-d":
                return output + "\n (NEEDED) Shared library: [libasan.so.8]"
            return output

        for mode, message in (("host", "resolve beside"), ("missing", "missing library"),
                              ("rpath", "own folder"), ("symbols", "Unstripped"),
                              ("dependency", "Unexpected ELF dependencies")):
            with self.subTest(mode=mode), patch.object(package_tool, "run", side_effect=broken):
                with self.assertRaisesRegex(ValueError, message):
                    package_tool.check_runtime(self.stage, set(), "strip")

    def test_embedded_developer_paths_and_architecture_are_rejected(self):
        executable = self.stage / package_tool.EXECUTABLE
        executable.write_bytes(b"\x7fELF\0/home/developer/project\0")
        with patch.object(package_tool, "run", side_effect=self.elf_output):
            with self.assertRaisesRegex(ValueError, "Embedded developer path"):
                package_tool.inspect_elf(executable, {"/home/developer/project"})
        executable.write_bytes(b"\x7fELF\0runtime")
        with patch.object(package_tool, "run", return_value="Machine: AArch64"):
            with self.assertRaisesRegex(ValueError, "architecture"):
                package_tool.inspect_elf(executable, set())


class AssetPackageTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="moorhuhn2-asset-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.build = self.root / "build/release"
        self.build.mkdir(parents=True)
        (self.root / "tools").mkdir()
        shutil.copyfile(REPOSITORY / "tools/pack_assets.py", self.root / "tools/pack_assets.py")
        write_assets(self.root / "assets")
        packer = package_tool.runpy.run_path(str(self.root / "tools/pack_assets.py"))
        for name, files in packer["collect_files"](self.root / "assets/manifest.txt").items():
            (self.build / f"{name}.pak").write_bytes(packer["pack_files"](files))
        self.addCleanup(patch.stopall)
        patch.object(package_tool, "REPOSITORY", self.root).start()

    def test_assets_reject_damaged_packages_and_unlisted_inputs(self):
        package_tool.checked_packages(self.build)
        (self.build / "audio.pak").write_bytes(b"damaged")
        with self.assertRaisesRegex(ValueError, "audio.pak"):
            package_tool.checked_packages(self.build)
        (self.root / "assets/unlisted.txt").write_text("unlisted")
        with self.assertRaisesRegex(ValueError, "unlisted"):
            package_tool.checked_packages(self.build)


if __name__ == "__main__":
    unittest.main()

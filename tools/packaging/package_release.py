#!/usr/bin/env python3
"""Create native release ZIPs from an already built Moorhuhn2 executable."""

from __future__ import annotations

import argparse
import os
from pathlib import Path, PurePosixPath
import platform
import plistlib
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile

if __package__:
    from . import linux_package
else:
    import linux_package


ROOT = Path(__file__).resolve().parents[2]
_ZIP_TIMESTAMP = (1980, 1, 1, 0, 0, 0)
_WINDOWS_SYSTEM_DLLS = {
    "advapi32", "avrt", "bcrypt", "cfgmgr32", "comctl32", "combase", "comdlg32",
    "crypt32", "d3d9", "d3d11", "d3d12", "d3dcompiler_47", "dbghelp", "dinput8",
    "dnsapi", "dwmapi", "dxgi", "gdi32", "hid", "imm32", "iphlpapi", "kernel32",
    "ksuser", "mf", "mfplat", "mfreadwrite", "mfuuid", "mmdevapi", "msacm32", "msimg32",
    "msvcrt", "netapi32", "normaliz", "ntdll", "ole32", "oleacc", "oleaut32",
    "powrprof", "propsys", "psapi", "rpcrt4", "secur32", "setupapi", "shell32",
    "shlwapi", "ucrtbase", "urlmon", "user32", "userenv", "usp10", "version", "winhttp",
    "wininet", "winmm", "winnsi", "winscard", "winspool", "wlanapi", "ws2_32",
    "wtsapi32", "xinput1_4", "xinput9_1_0",
}


class PackagingError(RuntimeError):
    """An input could not be turned into a runnable archive."""


def _run(command: list[str]) -> str:
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(("LD_", "DYLD_"))}
    try:
        result = subprocess.run(
            command, check=False, capture_output=True, text=True,
            env={**environment, "LC_ALL": "C"},
        )
    except OSError as error:
        raise PackagingError(f"could not run {command[0]!r}: {error}") from error
    if result.returncode:
        detail = result.stderr.strip() or result.stdout.strip()
        raise PackagingError(
            f"{command[0]} failed with exit status {result.returncode}"
            + (f": {detail}" if detail else "")
        )
    return result.stdout


def _file(path: Path, description: str) -> Path:
    try:
        resolved = path.resolve(strict=True)
    except (OSError, RuntimeError) as error:
        raise PackagingError(f"missing {description}: {path}") from error
    if not resolved.is_file():
        raise PackagingError(f"{description} is not a regular file: {path}")
    return resolved


def _linux(executable: Path, stage: Path, *, bundled: bool) -> None:
    # Keep the assets and shared libraries beside the executable for SDL_GetBasePath.
    shutil.copy2(executable, stage / "moorhuhn2")
    dependencies = linux_package.parse_ldd(_run(["ldd", str(executable)]))
    for name, source in sorted(dependencies.items()):
        if linux_package.HOST_GLIBC.fullmatch(name):
            continue
        if bundled or name in linux_package.SDL_LIBRARIES:
            shutil.copy2(_file(source, f"library {name!r}"), stage / name)


def _parse_imports(output: str) -> list[str]:
    names = re.findall(r"^\s*DLL Name:\s*(.*?)\s*$", output, re.MULTILINE)
    if any(not name or "/" in name or "\\" in name or not name.lower().endswith(".dll")
           for name in names):
        raise PackagingError("objdump reported an invalid DLL import name")
    return names


def _find_dll(name: str, directories: list[Path]) -> Path | None:
    for directory in directories:
        if directory.is_dir():
            for candidate in directory.iterdir():
                if candidate.name.lower() == name.lower() and candidate.is_file():
                    return candidate
    return None


def _windows_system_dll(name: str) -> bool:
    lower = name.lower()
    if lower.startswith(("api-ms-win-", "ext-ms-win-")):
        return True
    if lower.removesuffix(".dll") in _WINDOWS_SYSTEM_DLLS:
        return True
    windows = os.environ.get("SystemRoot") or os.environ.get("WINDIR")
    return bool(windows and _find_dll(name, [Path(windows) / "System32"]))


def _windows(executable: Path, stage: Path) -> None:
    shutil.copy2(executable, stage / "moorhuhn2.exe")
    search = [executable.parent] + [
        Path(entry) for entry in os.environ.get("PATH", "").split(os.pathsep) if entry
    ]
    pending = [executable]
    libraries: dict[str, Path] = {}
    while pending:
        source = pending.pop()
        for name in _parse_imports(_run(["objdump", "-p", str(source)])):
            if _windows_system_dll(name):
                continue
            dependency = _find_dll(name, [source.parent, *search])
            if dependency is None:
                raise PackagingError(f"missing DLL {name!r} imported by {source}")
            dependency = _file(dependency, f"DLL {name!r}")
            key = name.lower()
            if key in libraries:
                if libraries[key] != dependency:
                    raise PackagingError(f"conflicting paths for DLL {name!r}")
                continue
            libraries[key] = dependency
            shutil.copy2(dependency, stage / name)
            pending.append(dependency)


def _parse_otool(output: str) -> list[str]:
    names: list[str] = []
    for raw in output.splitlines()[1:]:
        if not raw.strip():
            continue
        match = re.fullmatch(r"\s+(.+?)\s+\(compatibility version .+\)", raw)
        if not match:
            raise PackagingError(f"unrecognized otool dependency: {raw!r}")
        names.append(match.group(1))
    return names


def _parse_rpaths(output: str) -> list[str]:
    return re.findall(
        r"\bcmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset \d+\)", output
    )


def _apple_system_path(name: str) -> bool:
    return name.startswith(("/usr/lib/", "/System/Library/"))


def _expand_macho_path(name: str, source: Path, executable: Path) -> Path | None:
    for token, directory in (("@loader_path", source.parent),
                             ("@executable_path", executable.parent)):
        if name == token:
            return directory
        if name.startswith(token + "/"):
            return directory / name[len(token) + 1:]
    return Path(name) if name.startswith("/") else None


def _macho_dependency(name: str, source: Path, executable: Path,
                      rpaths: list[Path]) -> Path:
    if name.startswith("@rpath/"):
        candidates = [path / name[len("@rpath/"):] for path in rpaths]
    else:
        expanded = _expand_macho_path(name, source, executable)
        candidates = [expanded] if expanded is not None else []
    for candidate in candidates:
        if candidate.is_file():
            return _file(candidate, f"library {name!r}")
    raise PackagingError(f"cannot resolve macOS library {name!r} imported by {source}")


def _macos(executable: Path, stage: Path, resources: dict[str, Path], version: str) -> None:
    app = stage / "Moorhuhn2.app"
    contents = app / "Contents"
    frameworks = contents / "Frameworks"
    frameworks.mkdir(parents=True)
    (contents / "MacOS").mkdir()
    resource_dir = contents / "Resources"
    resource_dir.mkdir()
    for name, source in resources.items():
        shutil.copy2(source, resource_dir / name)
    game = contents / "MacOS" / "Moorhuhn2"
    shutil.copy2(executable, game)
    game.chmod(0o755)
    with (contents / "Info.plist").open("wb") as info:
        plistlib.dump({
            "CFBundleExecutable": "Moorhuhn2",
            "CFBundleIdentifier": "org.moorhuhn2.desktop",
            "CFBundleName": "Moorhuhn2",
            "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": version,
            "CFBundleVersion": version,
            "LSMinimumSystemVersion": "13.3",
            "NSHighResolutionCapable": True,
        }, info)

    libraries: dict[str, Path] = {}
    pending = [(executable, game, [])]
    while pending:
        source, destination, inherited_rpaths = pending.pop()
        own_rpaths = _parse_rpaths(_run(["otool", "-l", str(source)]))
        rpaths = [
            expanded for name in own_rpaths
            if (expanded := _expand_macho_path(name, source, executable)) is not None
        ] + inherited_rpaths
        dependencies = _parse_otool(_run(["otool", "-L", str(source)]))
        own_names = set()
        if destination != game:
            own_names = set(_run(["otool", "-D", str(source)]).splitlines()[1:])
        for name in dependencies:
            if name in own_names or _apple_system_path(name):
                continue
            dependency = _macho_dependency(name, source, executable, rpaths)
            if _apple_system_path(str(dependency)):
                continue
            basename = dependency.name
            target = frameworks / basename
            if basename in libraries and libraries[basename] != dependency:
                raise PackagingError(f"conflicting macOS library name {basename!r}")
            if basename not in libraries:
                libraries[basename] = dependency
                shutil.copy2(dependency, target)
                _run(["install_name_tool", "-id", f"@rpath/{basename}", str(target)])
                pending.append((dependency, target, rpaths))
            prefix = "@executable_path/../Frameworks" if destination == game else "@loader_path"
            _run(["install_name_tool", "-change", name,
                  f"{prefix}/{basename}", str(destination)])
        # Every bundled dependency now uses an explicit relative load path.
        for name in own_rpaths:
            _run(["install_name_tool", "-delete_rpath", name, str(destination)])

    # Fix every load command before signing nested code, then the enclosing app.
    for basename in sorted(libraries):
        _run(["codesign", "--force", "--sign", "-", "--timestamp=none",
              str(frameworks / basename)])
    _run(["codesign", "--force", "--sign", "-", "--timestamp=none", str(app)])
    _run(["codesign", "--verify", "--deep", "--strict", str(app)])


def _zip_mode(name: str) -> int:
    path = PurePosixPath(name)
    executable = (path.name == "moorhuhn2" or path.parent.match("*.app/Contents/MacOS")
                  or path.suffix.lower() in {".exe", ".dll", ".dylib"}
                  or re.search(r"\.so(?:\.\d+)*$", path.name) is not None)
    return 0o755 if executable else 0o644


def _zip_tree(stage: Path, output: Path) -> None:
    # Fixed metadata keeps archives reproducible and independent of the host umask.
    temporary = output.with_suffix(".zip.tmp")
    try:
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(stage.rglob("*")):
                if not path.is_file():
                    continue
                name = path.relative_to(stage).as_posix()
                info = zipfile.ZipInfo(name, date_time=_ZIP_TIMESTAMP)
                info.create_system = 3
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = (stat.S_IFREG | _zip_mode(name)) << 16
                archive.writestr(info, path.read_bytes())
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)


def _architecture(value: str) -> str:
    normalized = {"amd64": "x86_64", "aarch64": "arm64"}.get(value.lower(), value.lower())
    if normalized not in {"x86_64", "arm64"}:
        raise PackagingError(f"unsupported release architecture: {value!r}")
    return normalized


def _release_inputs(build_dir: Path, *, allow_dirty: bool) -> tuple[dict[str, Path], str]:
    values = linux_package.cache_values(build_dir)
    if (Path(values.get("CMAKE_HOME_DIRECTORY", "")).resolve() != ROOT
            or values.get("CMAKE_BUILD_TYPE") != "Release"
            or not linux_package.cmake_boolean(values.get("CMAKE_INTERPROCEDURAL_OPTIMIZATION"))):
        raise PackagingError("package requires this checkout's Release distribution build")
    if (linux_package.cmake_boolean(values.get("MOORHUHN2_SANITIZERS"))
            or any("-fsanitize" in value for key, value in values.items()
                   if key.startswith("CMAKE_") and "FLAGS" in key)):
        raise PackagingError("release packages cannot include sanitizer builds")
    dirty = bool(_run(["git", "-C", str(ROOT), "status", "--porcelain",
                       "--untracked-files=all"]).strip())
    if dirty and not allow_dirty:
        raise PackagingError("source is dirty; commit first or use --allow-dirty for development")
    version_match = re.search(r"project\(moorhuhn2 VERSION (\d+\.\d+\.\d+)",
                              (ROOT / "CMakeLists.txt").read_text())
    if version_match is None:
        raise PackagingError("cannot read application version")
    resources = {
        "README.txt": _file(ROOT / "tools/packaging/README.txt", "player README"),
        "images.pak": _file(build_dir / "images.pak", "image package"),
        "audio.pak": _file(build_dir / "audio.pak", "audio package"),
    }
    linux_package.checked_packages(build_dir)
    return resources, version_match[1]


def package(build_dir: Path, output_dir: Path | None = None,
            arch: str | None = None, *, allow_dirty: bool = False) -> list[Path]:
    system = platform.system()
    if system not in {"Linux", "Windows", "Darwin"}:
        raise PackagingError(f"unsupported release platform: {system!r}")
    architecture = _architecture(arch or platform.machine())
    build_dir = build_dir.resolve()
    executable = _file(build_dir / ("moorhuhn2.exe" if system == "Windows" else "moorhuhn2"),
                       "built executable")
    resources, version = _release_inputs(build_dir, allow_dirty=allow_dirty)
    output_dir = (output_dir or ROOT / "dist").resolve()
    if output_dir == build_dir or build_dir in output_dir.parents or output_dir in build_dir.parents:
        raise PackagingError("build and output directories must be separate")
    output_dir.mkdir(parents=True, exist_ok=True)
    label = "macOS" if system == "Darwin" else system
    if system == "Linux":
        strip = linux_package.cache_values(build_dir)["CMAKE_STRIP"]
        forbidden_paths = {str(ROOT), str(build_dir), str(Path.home())}
    archives = []
    for bundled in ([False, True] if system == "Linux" else [True]):
        output = output_dir / f"moorhuhn2-{label}-{architecture}{'-bundled' if bundled else ''}.zip"
        with tempfile.TemporaryDirectory(prefix=".package-", dir=output_dir) as temporary:
            stage = Path(temporary)
            shutil.copy2(resources["README.txt"], stage / "README.txt")
            if system == "Linux":
                _linux(executable, stage, bundled=bundled)
                if bundled:
                    linux_package.check_runtime(stage, forbidden_paths, strip)
                else:
                    for name in [linux_package.EXECUTABLE, *sorted(linux_package.SDL_LIBRARIES)]:
                        linux_package.run([strip, "--strip-unneeded", str(stage / name)])
                        linux_package.inspect_elf(stage / name, forbidden_paths)
            elif system == "Windows":
                _windows(executable, stage)
            else:
                _macos(executable, stage, resources, version)
            if system != "Darwin":
                for name in resources.keys() - {"README.txt"}:
                    shutil.copy2(resources[name], stage / name)
            _zip_tree(stage, output)
        archives.append(output)
    return archives


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build/release")
    parser.add_argument("--output-dir", type=Path, help="defaults to dist/")
    parser.add_argument("--arch", help="archive architecture (defaults to the host architecture)")
    parser.add_argument("--allow-dirty", action="store_true", help="allow uncommitted source changes")
    args = parser.parse_args()
    try:
        for archive in package(args.build_dir, args.output_dir, args.arch, allow_dirty=args.allow_dirty):
            print(archive)
    except (PackagingError, ValueError, OSError, UnicodeError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

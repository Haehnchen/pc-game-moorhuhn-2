"""Verify the Linux x86_64 runtime and packed assets of a release build."""

import os
from pathlib import Path
import re
import runpy
import subprocess


REPOSITORY = Path(__file__).resolve().parents[2]
EXECUTABLE = "moorhuhn2"
SDL_LIBRARIES = {"libSDL3.so.0", "libSDL3_mixer.so.0"}
COMPILER_LIBRARIES = {"libstdc++.so.6", "libgcc_s.so.1"}
BUNDLED_LIBRARIES = SDL_LIBRARIES | COMPILER_LIBRARIES
# Graphics/audio drivers need the host's matching glibc and dynamic loader.
HOST_GLIBC = re.compile(
    r"^(?:ld-linux[^/]*|ld64\.so\.\d+|"
    r"lib(?:c|m|mvec|pthread|dl|rt|resolv|util|anl|BrokenLocale|"
    r"thread_db|nss_[^/]+)\.so(?:\.\d+)*)$"
)
_ADDRESS = r"\(0x[0-9a-fA-F]+\)"
_LDD_DEPENDENCY = re.compile(rf"^(?P<name>[^\s/]+)\s+=>\s+(?P<path>/.+?)\s+{_ADDRESS}$")
_LDD_ABSOLUTE = re.compile(rf"^(?P<path>/.+?)\s+{_ADDRESS}$")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def run(arguments):
    environment = {key: value for key, value in os.environ.items() if not key.startswith("LD_")}
    return subprocess.check_output(arguments, env={**environment, "LC_ALL": "C"}, text=True).strip()


def cache_values(build):
    values = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        match = re.match(r"([^/#][^:]*):[^=]+=(.*)", line)
        if match:
            values[match[1]] = match[2]
    return values


def cmake_boolean(value):
    # Preset JSON booleans are stored as TRUE/FALSE rather than option ON/OFF.
    normalized = str(value).upper()
    if normalized in {"1", "ON", "YES", "TRUE", "Y"}:
        return True
    if normalized in {"0", "OFF", "NO", "FALSE", "N", "IGNORE", "NOTFOUND", ""} or normalized.endswith("-NOTFOUND"):
        return False
    return None


def parse_ldd(output):
    dependencies = {}
    for raw in output.splitlines():
        line = raw.strip()
        if not line or line in {"statically linked", "not a dynamic executable"}:
            continue
        if re.fullmatch(rf"(?:linux-vdso|linux-gate)[^\s/]*\s+{_ADDRESS}", line):
            continue
        require(not re.fullmatch(r"[^\s/]+\s+=>\s+not\s+found", line),
                f"missing library reported by ldd: {line}")
        if match := _LDD_DEPENDENCY.fullmatch(line):
            name, path = match["name"], Path(match["path"])
        elif match := _LDD_ABSOLUTE.fullmatch(line):
            path = Path(match["path"])
            name = path.name
        else:
            raise ValueError(f"unrecognized ldd output: {raw!r}")
        require(dependencies.get(name, path) == path, f"ldd reports conflicting paths for {name!r}")
        dependencies[name] = path
    return dependencies


def glibc_requirements(versions):
    result = set()
    in_needs = False
    for line in versions.splitlines():
        if line.startswith("Version ") and " section " in line:
            in_needs = line.startswith("Version needs section ")
        if not in_needs:
            continue
        for name in re.findall(r"\bName: (GLIBC_\S+)", line):
            match = re.fullmatch(r"GLIBC_(\d+(?:\.\d+)+)", name)
            require(match is not None, f"Unrecognized GLIBC ABI requirement: {name}")
            result.add(match[1])
    return result


def inspect_elf(path, forbidden_paths):
    data = path.read_bytes()
    require(data.startswith(b"\x7fELF"), f"Not an ELF file: {path.name}")
    header = run(["readelf", "-h", str(path)])
    require("Advanced Micro Devices X86-64" in header and "ELF64" in header,
            f"Unsupported ELF architecture: {path.name}")
    for value in forbidden_paths:
        require(value.encode() not in data, f"Embedded developer path: {path.name}")
    require(not re.search(rb"/(?:home|root|Users)/[^\x00\n\r]+", data), f"Embedded home path: {path.name}")
    require(not re.search(rb"(?:^|\x00)/[^\x00\n\r]+\.(?:c|cc|cpp|h|hpp)\x00", data),
            f"Embedded absolute source path: {path.name}")
    return glibc_requirements(run(["readelf", "--version-info", str(path)]))


def dependencies(executable, stage):
    result = parse_ldd(run(["ldd", str(executable)]))
    require(result, f"Cannot inspect ELF dependencies: {executable.name}")
    for name, path in result.items():
        if name in BUNDLED_LIBRARIES:
            require(path.resolve() == (stage / name).resolve(),
                    f"Library does not resolve beside the executable: {name}")
        else:
            require(HOST_GLIBC.fullmatch(name), f"Unexpected system runtime: {name}")
    return result


def check_runtime(stage, forbidden_paths, strip):
    glibc_versions = set()
    resolved = {}
    for name in [EXECUTABLE, *sorted(BUNDLED_LIBRARIES)]:
        path = stage / name
        require(path.is_file() and not path.is_symlink(), f"Missing runtime file: {name}")
        run([strip, "--strip-unneeded", str(path)])
        dynamic = run(["readelf", "-d", str(path)])
        search_paths = re.findall(r"\((?:RUNPATH|RPATH)\).*?\[(.*?)\]", dynamic)
        if name in COMPILER_LIBRARIES:
            require(not search_paths, f"Unexpected compiler runtime search path: {name}")
        else:
            require(search_paths == ["$ORIGIN"], f"Runtime must search its own folder: {name}")
        needed = re.findall(r"\(NEEDED\).*?\[(.*?)\]", dynamic)
        require(all(entry in BUNDLED_LIBRARIES or HOST_GLIBC.fullmatch(entry) for entry in needed),
                f"Unexpected ELF dependencies: {name}")
        require(".symtab" not in run(["readelf", "-S", str(path)]), f"Unstripped runtime: {name}")
        if name == EXECUTABLE:
            resolved = dependencies(path, stage)
        glibc_versions.update(inspect_elf(path, forbidden_paths))
    require(resolved.keys() >= BUNDLED_LIBRARIES, "Executable does not use the complete bundled runtime")
    require(glibc_versions, "No GLIBC requirement found")


def payload_files(root):
    result = set()
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root).as_posix()
        require(not path.is_symlink(), f"Package symlink: {relative}")
        if path.is_dir():
            continue
        require(path.is_file(), f"Non-file payload: {relative}")
        require(not any(character in relative for character in "\n\r\\"), "Unsafe filename")
        result.add(relative)
    return result


def checked_packages(build):
    packer = runpy.run_path(str(REPOSITORY / "tools/pack_assets.py"))
    asset_root = REPOSITORY / "assets"
    groups = packer["collect_files"](asset_root / "manifest.txt")
    require(groups.keys() == {"images", "audio"}, "Unexpected MH2 asset package groups")
    expected_paths = {name for files in groups.values() for name, _ in files}
    # The Windows resource compiler embeds the icon separately from the game packages.
    require(payload_files(asset_root) - {"moorhuhn2.ico"} == expected_paths,
            "Source assets contain missing or unlisted files")
    for name, files in groups.items():
        filename = f"{name}.pak"
        require((build / filename).read_bytes() == packer["pack_files"](files),
                f"Built asset package differs: {filename}")

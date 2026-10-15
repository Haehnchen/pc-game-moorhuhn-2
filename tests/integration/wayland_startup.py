#!/usr/bin/env python3
"""Check initial presentation before focus on an owned, silent Wayland display.

Requires Weston 14 extracted under --weston-root, bwrap and PrivateRuntime's
X11 tools. No host compositor, hardware devices or package installation is used.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import socket
import struct
import tempfile
import time

from round_native import (REPOSITORY, PrivateRuntime, assert_closed, center_pointer,
                          source_title, wait_screen, wait_title)


def verify_devices(process) -> list[int]:
    """Inspect only children in our process group, including the bwrap payload."""
    payloads = []
    for entry in Path("/proc").iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            if os.getpgid(int(entry.name)) != process.pid:
                continue
            # The wrapper retains the parent's namespace; inspect sandbox children.
            if (entry / "ns/mnt").readlink() == Path("/proc/self/ns/mnt").readlink():
                continue
            for device in ("dri", "snd", "nvidia0", "nvidiactl"):
                if (entry / "root/dev" / device).exists():
                    raise RuntimeError(f"Hardware device exposed to private process {entry.name}")
            for descriptor in (entry / "fd").iterdir():
                target = str(descriptor.readlink())
                if target.startswith(("/dev/dri/", "/dev/snd/", "/dev/nvidia")):
                    raise RuntimeError(f"Hardware descriptor in private process {entry.name}")
            payloads.append(int(entry.name))
        except (FileNotFoundError, ProcessLookupError):
            continue
    if not payloads:
        raise RuntimeError("Private device namespace could not be verified")
    return payloads


def check(runtime, socket_dir: str, binary: Path, weston_root: Path) -> dict:
    bwrap = shutil.which("bwrap")
    if bwrap is None:
        raise RuntimeError("bwrap is required for private GPU/audio device isolation")
    libraries = next((weston_root / "usr/lib").glob("*/libweston-14.so.0")).parent
    backend = libraries / "libweston-14/x11-backend.so"
    shell = libraries / "weston/kiosk-shell.so"
    wrapper = [bwrap, "--bind", "/", "/", "--dev", "/dev", "--die-with-parent", "--"]
    environment = runtime.env
    environment.update(XDG_RUNTIME_DIR=socket_dir,
                       LD_LIBRARY_PATH=f"{libraries}:{libraries}/weston",
                       WESTON_MODULE_MAP=f"x11-backend.so={backend}",
                       WESTON_DATA_DIR=str(weston_root / "usr/share/weston"))
    endpoint = Path(socket_dir) / "mh2-test"
    if os.path.lexists(endpoint):
        raise RuntimeError("Refusing an existing Wayland socket")
    server = runtime._spawn(
        [*wrapper, weston_root / "usr/bin/weston", "--backend=x11", "--renderer=pixman",
         f"--shell={shell}", "--no-config", "--socket=mh2-test", "--width=640",
         "--height=480", "--idle-time=0"], "weston", environment=environment, lifetime=90)
    PrivateRuntime.wait_until(endpoint.is_socket, "owned Wayland socket", server)
    with socket.socket(socket.AF_UNIX) as connection:
        connection.connect(str(endpoint))
        peer_pid, peer_uid, _ = struct.unpack("3i", connection.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12))
    if peer_uid != os.getuid() or os.getpgid(peer_pid) != server.pid:
        raise RuntimeError("Wayland socket does not belong to our compositor")
    server_pids = verify_devices(server)
    window = int(re.search(r"window id (\d+)", server.log_path.read_text())[1])

    environment = runtime.env
    # SDL's Wayland software framebuffer still needs EGL to submit its buffer.
    # Mesa's CPU renderer and the private /dev prevent hardware access.
    environment.update(XDG_RUNTIME_DIR=socket_dir, WAYLAND_DISPLAY=endpoint.name,
                       SDL_VIDEODRIVER="wayland", SDL_VIDEO_DRIVER="wayland",
                       SDL_FRAMEBUFFER_ACCELERATION="opengles2", LIBGL_ALWAYS_SOFTWARE="1",
                       GALLIUM_DRIVER="llvmpipe", MESA_LOADER_DRIVER_OVERRIDE="swrast",
                       __EGL_VENDOR_LIBRARY_FILENAMES="/usr/share/glvnd/egl_vendor.d/50_mesa.json")
    app = runtime._spawn([*wrapper, binary],
                         "app", environment=environment, lifetime=75)
    try:
        # Observe the title before sending any input: its animation needs focus,
        # so this also verifies that first presentation mapped the Wayland window.
        title, oracle = source_title(REPOSITORY / "assets/manifest.txt")
        wait_screen(runtime, app, window, title, "wayland-first-title",
                    region=(0, 0, 320, 200), timeout=50)
        app_pids = verify_devices(app)
        center_pointer(runtime, window)
        wait_title(runtime, app, window, "wayland-title")

        runtime.xdotool(["windowminimize", window])
        time.sleep(.4)
        runtime.xdotool(["windowactivate", "--sync", window])
        wait_title(runtime, app, window, "wayland-resumed")
        runtime.xdotool(["key", "Escape"])
        assert_closed(runtime, app)
        return {"status": "passed", "binary": str(binary), "oracle": oracle,
                "compositor_pid": peer_pid, "private_device_pids": server_pids + app_pids}
    finally:
        runtime.screenshot("artifacts/final-display.png", window=window)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=REPOSITORY / "build/dev/moorhuhn2")
    parser.add_argument("--weston-root", type=Path, default=REPOSITORY / ".local/tools/weston/root")
    args = parser.parse_args()
    # Unix socket paths cannot exceed 107 bytes; normal case paths are longer.
    # This mode-0700 directory is still private and removed after owned cleanup.
    (REPOSITORY / ".local").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wl-", dir=REPOSITORY / ".local") as socket_dir:
        with PrivateRuntime(REPOSITORY, "wayland-startup", window_manager=True, screen=(800, 600)) as runtime:
            result = {"status": "failed"}
            try:
                result = check(runtime, socket_dir, args.binary.resolve(strict=True),
                               args.weston_root.resolve(strict=True))
            except BaseException as error:
                result["error"] = repr(error)
                raise
            finally:
                (runtime.case_dir / "artifacts/result.json").write_text(json.dumps(result, indent=2) + "\n")
    print("pass: Wayland maps without injected focus, exact title pixels, focus recovery, clean exit", flush=True)


if __name__ == "__main__":
    main()

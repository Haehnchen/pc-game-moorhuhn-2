"""Owned, silent X11 runtime for native integration checks.

Every application is launched in a new process group with an unavailable SDL
audio backend for offline fallback, or an explicit dummy SDL device.
Cases and their private storage remain available after cleanup.
"""
from __future__ import annotations

from collections.abc import Mapping, Sequence
import contextlib
from dataclasses import dataclass, field
from datetime import UTC, datetime
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import subprocess
import threading
import time
import uuid

from tools.image_pixels import RgbImage


DISABLED_AUDIO_DRIVER = "moorhuhn2-disabled"
_PRIVATE_ENV = {
    "DISPLAY", "WAYLAND_DISPLAY", "WAYLAND_SOCKET", "XAUTHORITY", "ICEAUTHORITY", "SESSION_MANAGER",
    "DBUS_SESSION_BUS_ADDRESS", "DBUS_SYSTEM_BUS_ADDRESS", "DBUS_STARTER_ADDRESS", "DBUS_STARTER_BUS_TYPE",
    "XMODIFIERS", "GTK_IM_MODULE", "QT_IM_MODULE",
    "PULSE_SERVER", "PULSE_CLIENTCONFIG", "PULSE_COOKIE", "PIPEWIRE_REMOTE",
    "PIPEWIRE_RUNTIME_DIR", "ALSA_CONFIG_PATH", "AUDIODEV", "AUDIODRIVER",
    "DESKTOP_SESSION", "GBM_BACKEND", "DRI_PRIME", "TMPDIR",
}
_PRIVATE_PREFIXES = ("SDL_", "XDG_", "LD_", "LIBGL_", "MESA_", "GALLIUM_",
                     "__GL_", "__GLX_", "__EGL_", "VK_")


def _private_variable(name: str) -> bool:
    return name in _PRIVATE_ENV or name.startswith(_PRIVATE_PREFIXES)


def _positive_seconds(value: float, name: str) -> float:
    value = float(value)
    if not math.isfinite(value) or value <= 0:
        raise ValueError(f"{name} must be finite and positive")
    return value


def _display_paths(number: int) -> tuple[Path, Path]:
    return Path(f"/tmp/.X11-unix/X{number}"), Path(f"/tmp/.X{number}-lock")


def _group_live(pgid: int) -> bool:
    # A child can survive its group leader. Exclude zombies, which cannot run or
    # retain X11 connections, rather than treating leader exit as full cleanup.
    for entry in Path("/proc").iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            fields = (entry / "stat").read_text().rsplit(")", 1)[1].split()
            if int(fields[2]) == pgid and fields[0] != "Z":
                return True
        except (FileNotFoundError, ProcessLookupError, PermissionError, IndexError):
            continue
    return False


@dataclass
class AppProcess:
    process: subprocess.Popen[str]
    command: tuple[str, ...]
    log_path: Path
    deadline: float | None
    timed_out: bool = False
    stopped: bool = False
    timer: threading.Timer | None = field(default=None, repr=False)
    lock: threading.RLock = field(default_factory=threading.RLock, repr=False)

    @property
    def pid(self) -> int:
        return self.process.pid

    def poll(self) -> int | None:
        return self.process.poll()


def _wait_for(app: AppProcess, timeout: float, probe, label: str, *, probe_before_exit: bool = False):
    deadline = time.monotonic() + _positive_seconds(timeout, "wait timeout")
    if app.deadline is not None:
        deadline = min(deadline, app.deadline)
    while True:
        if not probe_before_exit and app.poll() is not None:
            raise RuntimeError(f"{label} failed during startup; see {app.log_path}")
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(f"{label} startup timed out; see {app.log_path}")
        result = probe(min(remaining, 1.0))
        if result:
            return result
        if app.poll() is not None:
            raise RuntimeError(f"App exited before {label}; see {app.log_path}")
        time.sleep(min(0.025, max(0.0, deadline - time.monotonic())))


class PrivateRuntime:
    """Context manager retaining one case under repo/.local/private-runtime.

    `env` is a copy for inspection; application overrides cannot replace routes
    controlled by this runtime. Relative artifact paths are relative to case_dir.
    """

    def __init__(self, repo_root: Path | str, case_name: str, *,
                 window_manager: bool = False, display_number: int | None = None,
                 screen: tuple[int, int] = (1280, 960), startup_timeout: float = 5.0,
                 xvfb_program: str = "Xvfb", announce: bool = True):
        self.repo_root = Path(repo_root).resolve(strict=True)
        self.case_name = re.sub(r"[^A-Za-z0-9_.-]+", "-", case_name).strip("-.") or "case"
        self.window_manager = window_manager
        if display_number is not None and (not isinstance(display_number, int) or display_number < 100):
            raise ValueError("Private display number must be an integer >= 100")
        if len(screen) != 2 or any(not isinstance(value, int) or not 1 <= value <= 16384 for value in screen):
            raise ValueError("Screen dimensions must be integers between 1 and 16384")
        self.display_number = display_number
        self.screen = screen
        self.startup_timeout = _positive_seconds(startup_timeout, "startup_timeout")
        self.xvfb_program = xvfb_program
        self.announce = announce
        self.case_dir: Path | None = None
        self.display: str | None = None
        self.socket_path: Path | None = None
        self.lock_path: Path | None = None
        self.xvfb: AppProcess | None = None
        self.wm: AppProcess | None = None
        self._environment: dict[str, str] = {}
        self._processes: list[AppProcess] = []
        self._owned_endpoints: dict[Path, tuple[int, int]] = {}
        self._events_lock = threading.Lock()
        self._closed = False
        self._tool_counter = 0
        self.tool_paths: dict[str, str] = {}

    @property
    def env(self) -> dict[str, str]:
        return self._environment.copy()

    @property
    def xvfb_pid(self) -> int | None:
        return self.xvfb.pid if self.xvfb else None

    def _event(self, event: str, **details: object) -> None:
        if self.case_dir is None:
            return
        record = {"event": event, "monotonic": time.monotonic(), **details}
        with self._events_lock, (self.case_dir / "events.jsonl").open("a") as stream:
            stream.write(json.dumps(record, sort_keys=True) + "\n")

    def _prepare_case(self) -> None:
        parent = self.repo_root / ".local" / "private-runtime"
        if not parent.resolve().is_relative_to(self.repo_root):
            raise RuntimeError("Private runtime storage resolves outside the repository")
        parent.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now(UTC).strftime("%Y%m%dT%H%M%S.%fZ")
        self.case_dir = parent / f"{stamp}-{self.case_name}-{uuid.uuid4().hex[:8]}"
        self.case_dir.mkdir(mode=0o700)
        for name in ("runtime", "data", "config", "cache", "state", "tmp", "work", "logs", "artifacts"):
            (self.case_dir / name).mkdir(mode=0o700)
        environment = {key: value for key, value in os.environ.items() if not _private_variable(key)}
        pulse = self.case_dir / "config" / "pulse-client.conf"
        pulse.write_text("autospawn = no\nenable-shm = no\n")
        alsa = self.case_dir / "config" / "alsa-null.conf"
        alsa.write_text('pcm.!default { type null }\nctl.!default { type null }\n')
        environment.update({
            "XDG_RUNTIME_DIR": str(self.case_dir / "runtime"),
            "XDG_DATA_HOME": str(self.case_dir / "data"),
            "XDG_CONFIG_HOME": str(self.case_dir / "config"),
            "XDG_CACHE_HOME": str(self.case_dir / "cache"),
            "XDG_STATE_HOME": str(self.case_dir / "state"),
            "TMPDIR": str(self.case_dir / "tmp"),
            "DBUS_SESSION_BUS_ADDRESS": "unix:path=" + str(self.case_dir / "runtime" / "no-session-bus"),
            "DBUS_SYSTEM_BUS_ADDRESS": "unix:path=" + str(self.case_dir / "runtime" / "no-system-bus"),
            "XMODIFIERS": "@im=none",
            "PULSE_SERVER": "unix:" + str(self.case_dir / "runtime" / "no-pulse-socket"),
            "PULSE_CLIENTCONFIG": str(pulse),
            "PIPEWIRE_REMOTE": str(self.case_dir / "runtime" / "no-pipewire-socket"),
            "PIPEWIRE_RUNTIME_DIR": str(self.case_dir / "runtime"),
            "ALSA_CONFIG_PATH": str(alsa),
            "SDL_RENDER_DRIVER": "software", "SDL_FRAMEBUFFER_ACCELERATION": "0",
            "SDL_VIDEODRIVER": "x11", "SDL_VIDEO_DRIVER": "x11",
            "SDL_AUDIODRIVER": DISABLED_AUDIO_DRIVER, "SDL_AUDIO_DRIVER": DISABLED_AUDIO_DRIVER,
        })
        self._environment = environment
        required = {"Xvfb": self.xvfb_program, "xdpyinfo": "xdpyinfo", "xdotool": "xdotool", "import": "import", "convert": "convert"}
        if self.window_manager:
            required.update({"openbox": "openbox", "xprop": "xprop"})
        for name, executable in required.items():
            resolved = shutil.which(executable)
            if resolved is None:
                raise RuntimeError(f"Required private-runtime tool is missing: {executable}")
            self.tool_paths[name] = resolved
        (self.case_dir / "case.json").write_text(json.dumps({
            "case_name": self.case_name, "case_dir": str(self.case_dir), "screen": self.screen,
            "window_manager": self.window_manager, "tools": self.tool_paths,
            "audio_default": "offline with unavailable SDL backend",
            "private_environment": {key: value for key, value in environment.items() if _private_variable(key)},
        }, indent=2) + "\n")

    def _choose_display(self) -> None:
        directory = Path("/tmp/.X11-unix")
        if os.path.lexists(directory) and not stat.S_ISDIR(directory.lstat().st_mode):
            raise RuntimeError("X11 socket directory is not a real directory")
        numbers = [self.display_number] if self.display_number is not None else range(120, 512)
        for number in numbers:
            socket_path, lock_path = _display_paths(number)
            # lexists also rejects dangling symlinks at a display endpoint.
            if os.path.lexists(socket_path) or os.path.lexists(lock_path):
                if self.display_number is not None:
                    raise RuntimeError(f"Private display :{number} already exists; refusing connection")
                continue
            self.display_number = number
            self.display = f":{number}"
            self.socket_path, self.lock_path = socket_path, lock_path
            return
        raise RuntimeError("No unused private display found")

    def _spawn(self, command: Sequence[str | Path], label: str, *,
               environment: Mapping[str, str] | None = None, cwd: Path | None = None,
               lifetime: float | None = None, capture_stdout: bool = False) -> AppProcess:
        assert self.case_dir is not None
        command = tuple(str(value) for value in command)
        log_path = self.case_dir / "logs" / f"{label}.log"
        with log_path.open("w") as log:
            process = subprocess.Popen(command, cwd=cwd or self.case_dir / "work",
                                       env=dict(environment or self._environment), text=True,
                                       stdout=subprocess.PIPE if capture_stdout else log,
                                       stderr=log if capture_stdout else subprocess.STDOUT,
                                       stdin=subprocess.DEVNULL, start_new_session=True)
        app = AppProcess(process, command, log_path,
                         time.monotonic() + lifetime if lifetime is not None else None)
        self._processes.append(app)
        self._event("launched", pid=app.pid, pgid=app.pid, command=command,
                    log_path=str(log_path), lifetime=lifetime)
        if self.announce and not capture_stdout:
            print(f"private-runtime case={self.case_dir.name} pid={app.pid} log={log_path}", flush=True)
        if lifetime is not None:
            app.timer = threading.Timer(lifetime, self._expire, args=(app,))
            app.timer.daemon = True
            app.timer.start()
        return app

    def _expire(self, app: AppProcess) -> None:
        with app.lock:
            if app.stopped:
                return
            if app.process.poll() is None or _group_live(app.pid):
                app.timed_out = True
                self._event("deadline", pid=app.pid, log_path=str(app.log_path))
            try:
                self._stop(app)
            except RuntimeError as error:
                self._event("cleanup_error", pid=app.pid, error=str(error))

    def _stop(self, app: AppProcess, grace: float = 1.0) -> None:
        with app.lock:
            if app.stopped:
                return
            if app.timer is not None:
                app.timer.cancel()
            with contextlib.suppress(ProcessLookupError):
                os.killpg(app.pid, signal.SIGTERM)
            deadline = time.monotonic() + grace
            while _group_live(app.pid) and time.monotonic() < deadline:
                app.process.poll()
                time.sleep(0.025)
            if _group_live(app.pid):
                with contextlib.suppress(ProcessLookupError):
                    os.killpg(app.pid, signal.SIGKILL)
            try:
                app.process.wait(timeout=grace)
            except subprocess.TimeoutExpired as error:
                raise RuntimeError(f"Owned group {app.pid} did not terminate; see {app.log_path}") from error
            deadline = time.monotonic() + grace
            while _group_live(app.pid) and time.monotonic() < deadline:
                time.sleep(0.025)
            if _group_live(app.pid):
                raise RuntimeError(f"Owned child processes remain in group {app.pid}")
            app.stopped = True
            self._event("stopped", pid=app.pid, returncode=app.process.returncode, timed_out=app.timed_out)

    def verify_display(self) -> None:
        if self.xvfb is None or self.xvfb.poll() is not None:
            raise RuntimeError("Owned Xvfb process is not running")
        assert self.socket_path is not None and self.lock_path is not None
        try:
            lock_stat, socket_stat = self.lock_path.lstat(), self.socket_path.lstat()
            if not stat.S_ISREG(lock_stat.st_mode) or lock_stat.st_uid != os.getuid():
                raise RuntimeError("Private Xvfb lock is not an owned regular file")
            server_pid = int(self.lock_path.read_text().strip())
            if server_pid != self.xvfb.pid:
                raise RuntimeError("Private display lock PID does not match the spawned Xvfb")
            if not stat.S_ISSOCK(socket_stat.st_mode) or socket_stat.st_uid != os.getuid():
                raise RuntimeError("Private Xvfb endpoint is not an owned socket")
        except FileNotFoundError as error:
            raise RuntimeError("Private display socket or lock is missing") from error
        for path, info in ((self.lock_path, lock_stat), (self.socket_path, socket_stat)):
            identity = (info.st_dev, info.st_ino)
            if path in self._owned_endpoints and self._owned_endpoints[path] != identity:
                raise RuntimeError("Private display endpoint was replaced")
            self._owned_endpoints[path] = identity

    def _tool(self, name: str, arguments: Sequence[str | int | Path], *,
              timeout: float = 3.0, check: bool = True) -> subprocess.CompletedProcess[str]:
        timeout = _positive_seconds(timeout, "tool timeout")
        self.verify_display()  # Always establish ownership before an X11 connection.
        self._tool_counter += 1
        app = self._spawn([self.tool_paths[name], *map(str, arguments)],
                          f"tool-{self._tool_counter:03d}-{name}", capture_stdout=True)
        stdout = ""
        try:
            stdout, _ = app.process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired as error:
            self._event("tool_timeout", pid=app.pid)
            self._stop(app)
            stdout, _ = app.process.communicate(timeout=1.0)
            raise TimeoutError(f"{name} timed out; see {app.log_path}") from error
        finally:
            self._stop(app)
            self._processes.remove(app)
            with app.log_path.open("a") as log:
                log.write(stdout)
        result = subprocess.CompletedProcess(app.command, app.process.returncode, stdout, app.log_path.read_text())
        if check and result.returncode:
            raise subprocess.CalledProcessError(result.returncode, result.args, result.stdout, result.stderr)
        return result

    def start(self) -> PrivateRuntime:
        if self.case_dir is not None or self._closed:
            raise RuntimeError("PrivateRuntime cannot be started twice")
        try:
            self._prepare_case()
            self._choose_display()
            assert self.display is not None
            # Recheck immediately before spawning; never reuse or remove a foreign endpoint.
            assert self.socket_path is not None and self.lock_path is not None
            if os.path.lexists(self.socket_path) or os.path.lexists(self.lock_path):
                raise RuntimeError("Private display became occupied before launch")
            xvfb_command = [self.tool_paths["Xvfb"], self.display, "-screen", "0",
                            f"{self.screen[0]}x{self.screen[1]}x24", "-nolisten", "tcp",
                            "-extension", "GLX", "-noreset"]
            self.xvfb = self._spawn(xvfb_command, "xvfb")
            self._environment["DISPLAY"] = self.display

            def display_ready(timeout):
                if not (os.path.lexists(self.socket_path) and os.path.lexists(self.lock_path)):
                    return False
                return self._tool("xdpyinfo", [], timeout=timeout, check=False).returncode == 0

            _wait_for(self.xvfb, self.startup_timeout, display_ready, "Xvfb")
            if self.window_manager:
                self.wm = self._spawn([self.tool_paths["openbox"], "--sm-disable"], "openbox")

                def window_manager_ready(timeout):
                    result = self._tool("xprop", ["-root", "_NET_SUPPORTING_WM_CHECK"], timeout=timeout)
                    return re.search(r"window id # 0x[1-9a-fA-F][0-9a-fA-F]*", result.stdout)

                _wait_for(self.wm, self.startup_timeout, window_manager_ready, "Openbox")
            self._event("ready", display=self.display, xvfb_pid=self.xvfb_pid,
                        wm_pid=self.wm.pid if self.wm else None)
            return self
        except BaseException as error:
            self._event("failure", error=repr(error))
            try:
                self.close()
            except RuntimeError as cleanup_error:
                self._event("cleanup_error", error=str(cleanup_error))
            raise

    def __enter__(self) -> PrivateRuntime:
        return self.start()

    def launch_app(self, command: Sequence[str | Path], *, timeout: float = 10.0,
                   audio_mode: str = "offline", cwd: Path | str | None = None,
                   env: Mapping[str, str] | None = None) -> AppProcess:
        if self._closed:
            raise RuntimeError("Private runtime is closed")
        self.verify_display()
        timeout = _positive_seconds(timeout, "app timeout")
        if audio_mode not in ("offline", "dummy"):
            raise ValueError("audio_mode must be 'offline' or 'dummy'")
        if not command:
            raise ValueError("Application command must not be empty")
        overrides = dict(env or {})
        if any(_private_variable(key) for key in overrides):
            raise ValueError("Application environment cannot override private display/storage/audio guards")
        environment = {**self._environment, **overrides}
        driver = "dummy" if audio_mode == "dummy" else DISABLED_AUDIO_DRIVER
        environment.update(SDL_AUDIODRIVER=driver, SDL_AUDIO_DRIVER=driver)
        command = list(map(str, command))
        assert self.case_dir is not None
        working = Path(cwd).resolve() if cwd is not None else self.case_dir / "work"
        if not working.is_relative_to(self.case_dir) or not working.is_dir():
            raise ValueError("Application cwd must be an existing directory inside this private case")
        app = self._spawn(command, f"app-{sum(item.deadline is not None for item in self._processes) + 1:03d}",
                          environment=environment, cwd=working, lifetime=timeout)
        self._event("app_audio", pid=app.pid, audio_mode=audio_mode, driver=driver)
        return app

    def wait(self, app: AppProcess, *, timeout: float | None = None, check: bool = False) -> int:
        if app not in self._processes:
            raise ValueError("Process is not owned by this runtime")
        allowance = max(0.001, (app.deadline or time.monotonic()) - time.monotonic())
        if timeout is not None:
            allowance = min(allowance, _positive_seconds(timeout, "wait timeout"))
        try:
            code = app.process.wait(timeout=allowance)
        except subprocess.TimeoutExpired as error:
            app.timed_out = True
            self._event("wait_timeout", pid=app.pid, log_path=str(app.log_path))
            self._stop(app)
            raise TimeoutError(f"Application timed out; see {app.log_path}") from error
        self._stop(app)
        if app.timed_out:
            raise TimeoutError(f"Application exceeded its deadline; see {app.log_path}")
        if check and code:
            raise subprocess.CalledProcessError(code, app.command)
        return code

    def xdotool(self, arguments: Sequence[str | int], *, timeout: float = 3.0,
                check: bool = True) -> subprocess.CompletedProcess[str]:
        # xdotool can otherwise reconnect to an arbitrary X server mid-command.
        if any(str(value) in ("--display", "exec") or str(value).startswith("--display=") for value in arguments):
            raise ValueError("xdotool display changes and exec are not permitted")
        return self._tool("xdotool", arguments, timeout=timeout, check=check)

    def wait_for_window(self, *, pid: int, timeout: float = 5.0) -> int:
        app = next((item for item in self._processes if item.pid == pid and item.deadline is not None), None)
        if app is None:
            raise ValueError("Window PID must name an application owned by this runtime")

        def find_window(timeout):
            result = self.xdotool(["search", "--onlyvisible", "--pid", pid], timeout=timeout, check=False)
            if result.returncode == 0 and result.stdout.strip():
                return int(result.stdout.splitlines()[0])

        return _wait_for(app, timeout, find_window, "Application window")

    @staticmethod
    def wait_until(predicate, description: str, app: AppProcess, *, timeout: float = 5.0):
        # A final log record remains readable after the process has exited.
        return _wait_for(app, timeout, lambda remaining: predicate(), description, probe_before_exit=True)

    def screenshot(self, path: Path | str, *, window: int | None = None,
                   timeout: float = 3.0):
        assert self.case_dir is not None
        path = Path(path)
        if not path.is_absolute():
            path = self.case_dir / path
        path = path.resolve()
        if not path.is_relative_to(self.case_dir):
            raise ValueError("Screenshots must remain inside this private case")
        path.parent.mkdir(parents=True, exist_ok=True)
        self._tool("import", ["-window", "root" if window is None else window, "PNG:" + str(path)], timeout=timeout)
        image = RgbImage.load(path)
        self._event("screenshot", path=str(path), window=window, size=image.size)
        return image

    def close(self) -> None:
        if self._closed:
            return
        errors = []
        for app in reversed(self._processes):
            try:
                self._stop(app)
            except RuntimeError as error:
                errors.append(str(error))
        # SIGTERM normally lets Xvfb remove its files. A forced kill may leave
        # endpoints: remove only the exact inodes verified while our server lived.
        for path, identity in self._owned_endpoints.items():
            if not os.path.lexists(path):
                continue
            info = path.lstat()
            if (info.st_dev, info.st_ino) != identity or info.st_uid != os.getuid():
                errors.append(f"Refusing cleanup of replaced private display endpoint {path}")
                continue
            if path == self.lock_path and path.read_text().strip() != str(self.xvfb_pid):
                errors.append(f"Refusing cleanup of foreign private display lock {path}")
                continue
            if self.xvfb is not None and _group_live(self.xvfb.pid):
                errors.append(f"Refusing endpoint cleanup while Xvfb group {self.xvfb.pid} is alive")
                continue
            path.unlink()
        self._closed = True
        self._event("closed", errors=errors)
        if errors:
            raise RuntimeError("; ".join(errors))

    def __exit__(self, exception_type, exception, traceback) -> None:
        if exception is not None:
            self._event("failure", error=repr(exception))
        try:
            self.close()
        except RuntimeError as error:
            if exception is None:
                raise
            self._event("cleanup_error", error=str(error))

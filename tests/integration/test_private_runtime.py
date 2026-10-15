#!/usr/bin/env python3
"""Native-helper lifecycle tests using silent synthetic X11/Python processes."""
from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import time
import unittest
from unittest import mock

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY))
from tools.private_runtime import DISABLED_AUDIO_DRIVER, PrivateRuntime
from tools.image_pixels import RgbImage


# This is a small Xlib client, not the production app. It has no audio code and
# publishes its PID so the real input/window helper can find its private window.
WINDOW_CLIENT = r"""
import ctypes as c
import json
import os
import time
x = c.CDLL("libX11.so.6")
prototypes = {
    "XOpenDisplay": ([c.c_char_p], c.c_void_p),
    "XDefaultRootWindow": ([c.c_void_p], c.c_ulong),
    "XCreateSimpleWindow": ([c.c_void_p,c.c_ulong,c.c_int,c.c_int,c.c_uint,c.c_uint,c.c_uint,c.c_ulong,c.c_ulong], c.c_ulong),
    "XInternAtom": ([c.c_void_p,c.c_char_p,c.c_int], c.c_ulong),
    "XChangeProperty": ([c.c_void_p,c.c_ulong,c.c_ulong,c.c_ulong,c.c_int,c.c_int,c.c_void_p,c.c_int], c.c_int),
    "XStoreName": ([c.c_void_p,c.c_ulong,c.c_char_p], c.c_int),
    "XSelectInput": ([c.c_void_p,c.c_ulong,c.c_long], c.c_int),
    "XMapWindow": ([c.c_void_p,c.c_ulong], c.c_int),
    "XFlush": ([c.c_void_p], c.c_int),
    "XPending": ([c.c_void_p], c.c_int),
    "XNextEvent": ([c.c_void_p,c.c_void_p], c.c_int),
}
for name, (args, result) in prototypes.items():
    getattr(x, name).argtypes, getattr(x, name).restype = args, result
print(json.dumps({key: os.environ.get(key) for key in (
    "DISPLAY", "WAYLAND_DISPLAY", "WAYLAND_SOCKET", "XAUTHORITY", "DBUS_SESSION_BUS_ADDRESS",
    "DBUS_SYSTEM_BUS_ADDRESS", "XMODIFIERS", "GTK_IM_MODULE", "QT_IM_MODULE",
    "SDL_AUDIODRIVER", "SDL_AUDIO_DRIVER", "SDL_RENDER_DRIVER", "SDL_FRAMEBUFFER_ACCELERATION",
    "XDG_RUNTIME_DIR", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME",
    "PULSE_SERVER", "PIPEWIRE_REMOTE", "LD_PRELOAD")}), flush=True)
display = x.XOpenDisplay(None)
if not display:
    raise RuntimeError("Synthetic client cannot connect")
window = x.XCreateSimpleWindow(display, x.XDefaultRootWindow(display), 20, 20, 160, 120, 0, 0, 0xff0000)
pid = c.c_ulong(os.getpid())
x.XChangeProperty(display, window, x.XInternAtom(display, b"_NET_WM_PID", 0),
                  x.XInternAtom(display, b"CARDINAL", 0), 32, 0, c.byref(pid), 1)
x.XStoreName(display, window, b"Synthetic private window")
x.XSelectInput(display, window, 1 | 4)
x.XMapWindow(display, window)
x.XFlush(display)
print("window=" + str(window), flush=True)
while True:
    while x.XPending(display):
        event = (c.c_long * 24)()
        x.XNextEvent(display, c.byref(event))
        print("input=" + str(c.cast(event, c.POINTER(c.c_int))[0]), flush=True)
    time.sleep(0.01)
"""


def live(pid: int) -> bool:
    try:
        fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
        return fields[0] != "Z"
    except (FileNotFoundError, ProcessLookupError):
        return False


def wait_log(path: Path, text: str, timeout: float = 2.0) -> None:
    deadline = time.monotonic() + timeout
    while text not in path.read_text():
        if time.monotonic() >= deadline:
            raise AssertionError(f"Missing {text!r} in retained log {path}")
        time.sleep(0.01)


class PrivateRuntimeTests(unittest.TestCase):
    def test_wait_predicate_preserves_result_and_final_records(self):
        app = mock.Mock(deadline=None, log_path=Path("app.log"))
        app.poll.return_value = None
        predicate = mock.Mock(side_effect=[None, {"ready": True}])
        with mock.patch("tools.private_runtime.time.sleep"):
            self.assertEqual(PrivateRuntime.wait_until(predicate, "ready record", app), {"ready": True})
        app.poll.return_value = 0
        self.assertEqual(PrivateRuntime.wait_until(lambda: "closed", "final record", app), "closed")
        with self.assertRaisesRegex(RuntimeError, "App exited before missing record"):
            PrivateRuntime.wait_until(lambda: None, "missing record", app)

    def test_wait_predicate_honors_deadline_and_timeout_validation(self):
        app = mock.Mock(deadline=time.monotonic() - 1, log_path=Path("app.log"))
        app.poll.return_value = None
        with self.assertRaises(TimeoutError):
            PrivateRuntime.wait_until(lambda: None, "missing record", app, timeout=60)
        for timeout in (0, -1, float("inf"), float("nan")):
            with self.subTest(timeout=timeout), self.assertRaises(ValueError):
                PrivateRuntime.wait_until(lambda: True, "ready record", app, timeout=timeout)

    def runtime(self, name: str, **options) -> PrivateRuntime:
        return PrivateRuntime(REPOSITORY, "test-" + name, announce=False, **options)

    def assert_clean(self, runtime: PrivateRuntime) -> None:
        self.assertIsNotNone(runtime.case_dir)
        self.assertTrue((runtime.case_dir / "events.jsonl").is_file())
        if runtime.xvfb is not None:
            self.assertFalse(live(runtime.xvfb.pid))
            self.assertFalse(os.path.lexists(runtime.socket_path))
            self.assertFalse(os.path.lexists(runtime.lock_path))
        if runtime.wm is not None:
            self.assertFalse(live(runtime.wm.pid))
        self.assertEqual(json.loads((runtime.case_dir / "events.jsonl").read_text().splitlines()[-1])["event"], "closed")

    def test_private_window_input_screenshot_and_normal_cleanup(self):
        inherited = {
            "DISPLAY": ":0", "WAYLAND_DISPLAY": "wayland-host", "WAYLAND_SOCKET": "3", "XAUTHORITY": "/host/auth",
            "PULSE_SERVER": "unix:/host/pulse", "PIPEWIRE_REMOTE": "host-pipewire",
            "DBUS_SESSION_BUS_ADDRESS": "unix:path=/host/bus", "DBUS_SYSTEM_BUS_ADDRESS": "unix:path=/host/system-bus",
            "XMODIFIERS": "@im=ibus", "GTK_IM_MODULE": "ibus", "QT_IM_MODULE": "ibus",
            "SDL_AUDIODRIVER": "alsa",
            "SDL_AUDIO_DRIVER": "pipewire", "SDL_RENDER_DRIVER": "opengl",
            "SDL_FRAMEBUFFER_ACCELERATION": "1", "LD_PRELOAD": "/host/preload",
        }
        runtime = self.runtime("normal")
        with mock.patch.dict(os.environ, inherited), runtime:
            runtime.verify_display()
            self.assertNotEqual(runtime.display, ":0")
            display_info = (runtime.case_dir / "logs/tool-001-xdpyinfo.log").read_text()
            self.assertNotIn("GLX", display_info)
            for name in ("runtime", "data", "config", "cache", "tmp", "work"):
                self.assertEqual((runtime.case_dir / name).stat().st_mode & 0o777, 0o700)
            app = runtime.launch_app([sys.executable, "-u", "-c", WINDOW_CLIENT], timeout=15.0)
            self.assertEqual(app.command, (sys.executable, "-u", "-c", WINDOW_CLIENT))
            self.assertEqual(os.getpgid(app.pid), app.pid)
            self.assertEqual(os.getsid(app.pid), app.pid)
            window = runtime.wait_for_window(pid=app.pid, timeout=10.0)
            environment = json.loads(app.log_path.read_text().splitlines()[0])
            self.assertEqual(environment["DISPLAY"], runtime.display)
            for name in ("WAYLAND_DISPLAY", "WAYLAND_SOCKET", "XAUTHORITY", "LD_PRELOAD", "GTK_IM_MODULE",
                         "QT_IM_MODULE"):
                self.assertIsNone(environment[name])
            for name in ("SDL_AUDIODRIVER", "SDL_AUDIO_DRIVER"):
                self.assertEqual(environment[name], DISABLED_AUDIO_DRIVER)
            for name in ("XDG_RUNTIME_DIR", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME"):
                self.assertTrue(Path(environment[name]).is_relative_to(runtime.case_dir))
            self.assertEqual(environment["SDL_RENDER_DRIVER"], "software")
            self.assertEqual(environment["SDL_FRAMEBUFFER_ACCELERATION"], "0")
            diagnostic = json.loads((runtime.case_dir / "case.json").read_text())
            self.assertEqual(diagnostic["private_environment"]["SDL_FRAMEBUFFER_ACCELERATION"], "0")
            self.assertEqual(environment["XMODIFIERS"], "@im=none")
            self.assertEqual(diagnostic["private_environment"]["XMODIFIERS"], "@im=none")
            for name in ("DBUS_SESSION_BUS_ADDRESS", "DBUS_SYSTEM_BUS_ADDRESS"):
                endpoint = environment[name]
                self.assertTrue(endpoint.startswith("unix:path="))
                socket = Path(endpoint.removeprefix("unix:path="))
                self.assertTrue(socket.is_relative_to(runtime.case_dir / "runtime"))
                self.assertFalse(socket.exists())
                self.assertEqual(diagnostic["private_environment"][name], endpoint)
                with self.assertRaisesRegex(ValueError, "cannot override"):
                    runtime.launch_app([sys.executable, "-c", "pass"], env={name: "unix:path=/host/bus"})
            self.assertNotIn("/host/", environment["PULSE_SERVER"])
            self.assertNotIn("host-pipewire", environment["PIPEWIRE_REMOTE"])
            geometry = runtime.xdotool(["getwindowgeometry", "--shell", window]).stdout
            self.assertIn("WIDTH=160", geometry)
            self.assertIn("HEIGHT=120", geometry)
            runtime.xdotool(["windowfocus", window])
            runtime.xdotool(["key", "--window", window, "a"])
            runtime.xdotool(["mousemove", "--window", window, 50, 50, "click", 1])
            wait_log(app.log_path, "input=2")
            wait_log(app.log_path, "input=4")
            captured = runtime.screenshot("artifacts/client.png", window=window)
            self.assertEqual(captured.size, (160, 120))
            self.assertEqual(captured.pixel((80, 60)), (255, 0, 0))
            copied = runtime.case_dir / "artifacts" / "copied.png"
            captured.save(copied)
            self.assertEqual(RgbImage.load(copied), captured)
            self.assertIsNone(app.poll())
        self.assertFalse(live(app.pid))
        self.assertTrue((runtime.case_dir / "artifacts/client.png").is_file())
        self.assert_clean(runtime)

    def test_optional_window_manager_is_private_and_cleaned(self):
        runtime = self.runtime("wm", window_manager=True)
        with runtime:
            app = runtime.launch_app([sys.executable, "-u", "-c", WINDOW_CLIENT], timeout=15.0)
            window = runtime.wait_for_window(pid=app.pid, timeout=10.0)
            runtime.xdotool(["windowactivate", "--sync", window])
            self.assertEqual(int(runtime.xdotool(["getactivewindow"]).stdout), window)
            self.assertTrue(live(runtime.wm.pid))
        self.assert_clean(runtime)

    def test_occupied_display_refused_without_removing_owner(self):
        owner = self.runtime("display-owner")
        with owner:
            number = owner.display_number
            other = self.runtime("occupied-refusal", display_number=number)
            with self.assertRaisesRegex(RuntimeError, "already exists"):
                other.start()
            owner.verify_display()
            self.assertTrue(live(owner.xvfb.pid))
            self.assertTrue(owner.socket_path.is_socket())
            self.assertIsNone(other.xvfb)
        self.assert_clean(owner)
        self.assert_clean(other)

    def test_dangling_endpoint_symlink_refused(self):
        runtime = self.runtime("dangling-refusal", display_number=499)
        # Endpoint discovery uses a private fixture, so this test never edits a
        # shared /tmp display lock and never connects to another server.
        fixture = REPOSITORY / ".local" / "private-runtime" / ("symlink-fixture-" + str(os.getpid()))
        fixture.mkdir(mode=0o700, exist_ok=False)
        endpoint = fixture / "socket"
        endpoint.symlink_to(fixture / "missing")
        try:
            with mock.patch("tools.private_runtime._display_paths", return_value=(endpoint, fixture / "lock")):
                with self.assertRaisesRegex(RuntimeError, "already exists"):
                    runtime.start()
            self.assertTrue(endpoint.is_symlink())
            self.assertIsNone(runtime.xvfb)
        finally:
            endpoint.unlink()
            fixture.rmdir()
        self.assert_clean(runtime)

    def test_startup_failure_retains_log_and_cleans(self):
        runtime = self.runtime("startup-failure", xvfb_program="/bin/false")
        with self.assertRaisesRegex(RuntimeError, "failed during startup"):
            runtime.start()
        self.assertTrue((runtime.case_dir / "logs/xvfb.log").is_file())
        self.assert_clean(runtime)

    def test_startup_timeout_retains_output_and_cleans(self):
        fixture = REPOSITORY / ".local" / "private-runtime" / ("timeout-fixture-" + str(os.getpid()))
        fixture.mkdir(mode=0o700, exist_ok=False)
        executable = fixture / "not-xvfb"
        executable.write_text("#!/bin/sh\nprintf 'synthetic stalled startup\\n'\nexec sleep 30\n")
        executable.chmod(0o700)
        runtime = self.runtime("startup-timeout", xvfb_program=str(executable), startup_timeout=0.1)
        try:
            with self.assertRaisesRegex(TimeoutError, "startup timed out"):
                runtime.start()
            self.assertIn("synthetic stalled startup", (runtime.case_dir / "logs/xvfb.log").read_text())
        finally:
            executable.unlink()
            fixture.rmdir()
        self.assert_clean(runtime)

    def test_leader_exit_cleans_surviving_child(self):
        runtime = self.runtime("orphan-child")
        with runtime:
            script = ("import subprocess,sys; "
                      "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)']); "
                      "print('child='+str(child.pid),flush=True)")
            app = runtime.launch_app([sys.executable, "-u", "-c", script], timeout=3.0)
            self.assertEqual(runtime.wait(app, check=True), 0)
            child = int(app.log_path.read_text().split("child=")[1].splitlines()[0])
            self.assertFalse(live(child))
        self.assert_clean(runtime)

    def test_pid_guard_blocks_input_before_connection(self):
        runtime = self.runtime("pid-guard")
        with runtime:
            fake_lock = runtime.case_dir / "wrong-lock"
            fake_lock.write_text("1\n")
            with mock.patch.object(runtime, "lock_path", fake_lock):
                with self.assertRaisesRegex(RuntimeError, "PID does not match"):
                    runtime.xdotool(["getmouselocation"])
            runtime.verify_display()
        self.assert_clean(runtime)

    def test_crash_and_explicit_dummy_audio(self):
        runtime = self.runtime("crash-dummy")
        with runtime:
            script = "import json,os,sys; print(json.dumps([os.environ['SDL_AUDIODRIVER'],os.environ['SDL_AUDIO_DRIVER'],sys.argv])); sys.exit(17)"
            app = runtime.launch_app([sys.executable, "-u", "-c", script], audio_mode="dummy", timeout=3.0)
            with self.assertRaises(subprocess.CalledProcessError) as caught:
                runtime.wait(app, check=True)
            self.assertEqual(caught.exception.returncode, 17)
            drivers = json.loads(app.log_path.read_text())
            self.assertEqual(drivers[:2], ["dummy", "dummy"])
            self.assertEqual(drivers[2], ["-c"])
            self.assertFalse(live(app.pid))
        self.assert_clean(runtime)

    def test_offline_audio_guards_preserve_the_command(self):
        runtime = self.runtime("unavailable-audio")
        inherited = {"SDL_AUDIODRIVER": "alsa", "SDL_AUDIO_DRIVER": "pipewire"}
        script = ("import json,os,sys; print(json.dumps({'drivers':"
                  "[os.environ['SDL_AUDIODRIVER'],os.environ['SDL_AUDIO_DRIVER']],"
                  "'arguments':sys.argv}))")
        with mock.patch.dict(os.environ, inherited), runtime:
            command = [sys.executable, "-u", "-c", script]
            app = runtime.launch_app(command, timeout=3.0)
            self.assertEqual(runtime.wait(app, check=True), 0)
            actual = json.loads(app.log_path.read_text())
            self.assertEqual(actual["drivers"], [DISABLED_AUDIO_DRIVER] * 2)
            self.assertEqual(actual["arguments"], ["-c"])
            self.assertEqual(app.command, tuple(command))
            for name in ("SDL_AUDIODRIVER", "SDL_AUDIO_DRIVER", "PULSE_SERVER", "XDG_DATA_HOME"):
                with self.assertRaisesRegex(ValueError, "cannot override"):
                    runtime.launch_app([sys.executable, "-c", "pass"], env={name: "/host"})
            events = [json.loads(line) for line in (runtime.case_dir / "events.jsonl").read_text().splitlines()]
            audio = [event for event in events if event["event"] == "app_audio"]
            self.assertEqual([event["audio_mode"] for event in audio], ["offline"])
            self.assertTrue(all(event["driver"] == DISABLED_AUDIO_DRIVER for event in audio))
        self.assert_clean(runtime)

    def test_automatic_deadline_kills_group_and_retains_failure(self):
        runtime = self.runtime("timeout")
        with runtime:
            script = ("import signal,subprocess,sys,time; "
                      "signal.signal(signal.SIGTERM,signal.SIG_IGN); "
                      "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)']); "
                      "print('child='+str(child.pid),flush=True); time.sleep(30)")
            app = runtime.launch_app([sys.executable, "-u", "-c", script], timeout=0.2)
            wait_log(app.log_path, "child=")
            child = int(app.log_path.read_text().split("child=")[1].splitlines()[0])
            # Observe asynchronous enforcement without calling runtime.wait first.
            deadline = time.monotonic() + 3.0
            while live(app.pid) and time.monotonic() < deadline:
                time.sleep(0.02)
            self.assertFalse(live(app.pid))
            self.assertFalse(live(child))
            with self.assertRaises(TimeoutError):
                runtime.wait(app)
            self.assertIn('"event": "deadline"', (runtime.case_dir / "events.jsonl").read_text())
        self.assert_clean(runtime)

    def test_tool_timeout_retains_partial_output(self):
        runtime = self.runtime("tool-timeout")
        with runtime:
            with self.assertRaisesRegex(TimeoutError, "xdotool timed out"):
                runtime.xdotool(["getmouselocation", "search", "--sync", "--name", "Missing synthetic title"], timeout=0.1)
            logs = sorted((runtime.case_dir / "logs").glob("*xdotool.log"))
            self.assertIn("x:", logs[-1].read_text())
            self.assertIn("y:", logs[-1].read_text())
            self.assertIn("x:", runtime.xdotool(["getmouselocation"]).stdout)
        self.assert_clean(runtime)

    def test_context_exception_and_override_refusal(self):
        runtime = self.runtime("exception")
        with self.assertRaisesRegex(RuntimeError, "synthetic failure"):
            with runtime:
                for env in ({"DISPLAY": ":0"}, {"SDL_AUDIO_DRIVER": "alsa"}, {"XDG_DATA_HOME": "/host"}):
                    with self.assertRaisesRegex(ValueError, "cannot override"):
                        runtime.launch_app([sys.executable, "-c", "pass"], env=env)
                with self.assertRaises(ValueError):
                    runtime.xdotool(["--display=:0", "getmouselocation"])
                with self.assertRaises(ValueError):
                    runtime.screenshot(REPOSITORY / "outside-case.png")
                app = runtime.launch_app([sys.executable, "-c", "import time; time.sleep(30)"], timeout=5.0,
                                         env={"ASAN_OPTIONS": "detect_leaks=1"})
                raise RuntimeError("synthetic failure")
        self.assertFalse(live(app.pid))
        self.assertIn("synthetic failure", (runtime.case_dir / "events.jsonl").read_text())
        self.assert_clean(runtime)


if __name__ == "__main__":
    unittest.main(verbosity=2)

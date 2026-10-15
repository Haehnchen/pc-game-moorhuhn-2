#!/usr/bin/env python3
"""Short production lifecycle check using visible screens and the saved score file."""

import argparse
import json
from pathlib import Path
import time

from round_native import (REPOSITORY, PrivateRuntime, assert_closed, center_pointer,
                          click, close_window, draw_text, save_path, source_screen,
                          wait_screen, wait_title)


def results_screen():
    expected = source_screen("gfx/scorescreen")
    font = source_screen("gfx/font")
    # Wait for a settled score row, not the background already revealed during
    # round_exit: entering results clears the queued keys after that animation.
    for text, x in (("Frank", 220), ("  250", 400)):
        expected = draw_text(expected, font, text, x, 150)
    return expected


def run(runtime, binary):
    app = runtime.launch_app([binary], timeout=60)
    window = runtime.wait_for_window(pid=app.pid, timeout=10)
    center_pointer(runtime, window)
    wait_title(runtime, app, window)

    click(runtime, window, 300, 455)
    wait_screen(runtime, app, window, source_screen("gfx/disclaimer"), "disclaimer")
    runtime.xdotool(["key", "space"])
    center_pointer(runtime, window)
    wait_title(runtime, app, window, "title-return")

    runtime.xdotool(["key", "space"])
    # The original round introduction lasts under three seconds. Its scene is
    # animated; the static boss screen then confirms hunting accepts input.
    time.sleep(3.5)
    runtime.xdotool(["key", "b"])
    boss = source_screen("gfx/Bossscreen")
    wait_screen(runtime, app, window, boss, "boss", masks=((320, 240, 352, 272),))
    runtime.xdotool(["windowminimize", window])
    time.sleep(.25)
    runtime.xdotool(["windowactivate", "--sync", window])
    wait_screen(runtime, app, window, boss, "boss-resumed", masks=((320, 240, 352, 272),))
    runtime.xdotool(["key", "b"])
    time.sleep(.15)
    runtime.xdotool(["key", "Escape"])
    wait_screen(runtime, app, window, results_screen(), "results", region=(200, 140, 560, 190))
    runtime.xdotool(["key", "Escape"])
    wait_title(runtime, app, window, "title-after-round")
    saved = save_path(runtime).read_bytes()
    close_window(runtime, window)
    assert_closed(runtime, app)
    if save_path(runtime).read_bytes() != saved:
        raise RuntimeError("Normal shutdown changed the saved round result")
    defaults = (REPOSITORY / "tests/fixtures/behavior/ui/original-default.sav").read_bytes()
    if saved != defaults:
        raise RuntimeError("A zero-score round changed the default highscore table")
    app = runtime.launch_app([binary], timeout=30, audio_mode="dummy")
    window = runtime.wait_for_window(pid=app.pid, timeout=10)
    center_pointer(runtime, window)
    wait_title(runtime, app, window, "title-restart")
    runtime.xdotool(["key", "Escape"])
    assert_closed(runtime, app)
    if save_path(runtime).read_bytes() != saved:
        raise RuntimeError("Restart changed the saved highscore table")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=REPOSITORY / "build/dev/moorhuhn2")
    args = parser.parse_args()
    binary = args.binary.resolve(strict=True)
    with PrivateRuntime(REPOSITORY, "lifecycle", window_manager=True, screen=(640, 480)) as runtime:
        result = {"status": "failed"}
        try:
            run(runtime, binary)
            result["status"] = "passed"
        except BaseException as error:
            result["error"] = repr(error)
            raise
        finally:
            (runtime.case_dir / "artifacts/result.json").write_text(json.dumps(result, indent=2) + "\n")
            print(f"lifecycle logs: {runtime.case_dir}", flush=True)


if __name__ == "__main__":
    main()

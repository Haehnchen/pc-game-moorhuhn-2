#!/usr/bin/env python3
"""Check production startup, title pixels and private save storage without app instrumentation."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
import time

from PIL import Image, ImageChops, ImageDraw

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY))

from tools.private_runtime import PrivateRuntime
from tools.pack_assets import read_manifest


def rgb_pixels(image: Image.Image):
    data = image.tobytes()
    return zip(data[0::3], data[1::3], data[2::3], strict=True)


def source_title(manifest: Path) -> tuple[Image.Image, dict]:
    document = read_manifest(manifest)
    matches = [entry for entry in document["images"] if entry["id"] == "gfx/mainscreen"]
    if len(matches) != 1:
        raise RuntimeError("Expected exactly one main-screen asset")
    entry = matches[0]
    title_path = (manifest.parent / entry["png"]["path"]).resolve(strict=True)
    if not title_path.is_relative_to(manifest.parent.resolve()):
        raise RuntimeError("Title image escapes the asset root")
    with Image.open(title_path) as source:
        image = source.convert("RGB")
    if image.size != (640, 480):
        raise RuntimeError("Expected the supplied MH2 640x480 main screen")
    converted = Image.new("RGB", image.size)
    converted.putdata([(((r >> 3) * 255 + 15) // 31,
                        ((g >> 2) * 255 + 31) // 63, ((b >> 3) * 255 + 15) // 31)
                       for r, g, b in rgb_pixels(image)])
    image = converted
    # The restored Extras route is an intentional title addition. Compose its
    # source font independently; keep all other background pixels exact.
    font_entry = next(entry for entry in document["images"] if entry["id"] == "gfx/font")
    font_path = manifest.parent / font_entry["png"]["path"]
    with Image.open(font_path) as source:
        font = source.convert("RGB")
    red = [((value >> 3) * 255 + 15) // 31 for value in range(256)]
    green = [((value >> 2) * 255 + 31) // 63 for value in range(256)]
    for index, character in enumerate("Extras"):
        glyph = ord(character) - ord('A') + 31 if character.isupper() else ord(character) - ord('a') + 57
        cell = font.crop((0, glyph * 50, 30, (glyph + 1) * 50))
        mask = Image.new("L", cell.size)
        mask.putdata([0 if ((r >> 3) << 11 | (g >> 2) << 5 | b >> 3) == 0xf81f else 255
                      for r, g, b in rgb_pixels(cell)])
        image.paste(cell.point(red + green + red), (480 + index * 20, 375), mask)
    return image, {"title": str(title_path), "font": str(font_path)}


def geometry(runtime: PrivateRuntime, window: int) -> dict[str, int]:
    output = runtime.xdotool(["getwindowgeometry", "--shell", window]).stdout
    values = {key: int(value) for key, value in re.findall(r"^(X|Y|WIDTH|HEIGHT)=(-?\d+)$", output, re.MULTILINE)}
    if set(values) != {"X", "Y", "WIDTH", "HEIGHT"}:
        raise RuntimeError(f"Incomplete private client geometry: {output}")
    return values


def source_screen(asset_id):
    manifest = read_manifest(REPOSITORY / "assets/manifest.txt")
    entry = next(item for item in manifest["images"] if item["id"] == asset_id)
    with Image.open(REPOSITORY / "assets" / entry["png"]["path"]) as image:
        image = image.convert("RGB")
    red = [((value >> 3) * 255 + 15) // 31 for value in range(256)]
    green = [((value >> 2) * 255 + 31) // 63 for value in range(256)]
    return image.point(red + green + red)


def wait_screen(runtime, app, window, expected, name, *, masks=(), region=None, timeout=30):
    """Observe ordinary window pixels; preserve the last mismatch for inspection."""
    size = geometry(runtime, window)
    width, height = size["WIDTH"], size["HEIGHT"]
    scale = min(width // 640, height // 480)
    if scale < 1:
        raise RuntimeError("Native screen checks require at least 640x480")
    x, y = (width - 640 * scale) // 2, (height - 480 * scale) // 2
    oracle = Image.new("RGB", (width, height))
    oracle.paste(expected.resize((640 * scale, 480 * scale), Image.Resampling.NEAREST), (x, y))

    def matches():
        actual = runtime.screenshot(f"artifacts/{name}.png", window=window)
        if actual.size != oracle.size:
            return False
        difference = ImageChops.difference(actual, oracle)
        draw = ImageDraw.Draw(difference)
        for left, top, right, bottom in masks:
            draw.rectangle((x + left * scale, y + top * scale,
                            x + right * scale - 1, y + bottom * scale - 1), fill=0)
        if region:
            left, top, right, bottom = region
            difference = difference.crop((x + left * scale, y + top * scale,
                                          x + right * scale, y + bottom * scale))
        if difference.getbbox() is None:
            return True
        difference.save(runtime.case_dir / "artifacts" / f"{name}-difference.png")
        time.sleep(.1)
        return False

    PrivateRuntime.wait_until(matches, name, app, timeout=timeout)


def wait_title(runtime, app, window, name="title"):
    title, _ = source_title(REPOSITORY / "assets/manifest.txt")
    wait_screen(runtime, app, window, title, name,
                masks=((320, 240, 352, 272), (585, 426, 640, 480)))


def center_pointer(runtime, window):
    size = geometry(runtime, window)
    runtime.xdotool(["mousemove", "--window", window, size["WIDTH"] // 2, size["HEIGHT"] // 2])


def save_path(runtime):
    return Path(runtime.env["XDG_DATA_HOME"]) / "Moorhuhn2/Moorhuhn2/moorhuhn2.sav"


def assert_closed(runtime: PrivateRuntime, app) -> None:
    if runtime.wait(app, timeout=10.0) != 0:
        raise RuntimeError(f"App did not close cleanly; see {app.log_path}")
    text = app.log_path.read_text(encoding="utf-8", errors="replace")
    unexpected = "\n".join(line for line in text.splitlines()
                           if not line.startswith("moorhuhn2: audio unavailable, continuing with offline mixing: "))
    if any(marker in unexpected for marker in ("AddressSanitizer", "LeakSanitizer", "UndefinedBehaviorSanitizer", "runtime error:", "moorhuhn2:", "FAIL ")):
        raise RuntimeError(f"App error or sanitizer diagnostic in {app.log_path}")
    remaining = runtime.xdotool(["search", "--onlyvisible", "--pid", app.pid], check=False)
    if remaining.returncode == 0 or remaining.stdout.strip():
        raise RuntimeError("Closed app left a visible private window")


def click(runtime: PrivateRuntime, window: int, x: int, y: int, button: int = 1) -> None:
    runtime.xdotool(["windowactivate", window])
    runtime.xdotool(["mousemove", "--window", window, x, y])
    runtime.xdotool(["mousedown", button])
    try:
        # The installed xdotool click command uses a 12-microsecond pulse.
        # Separate its physical edges across app updates without filtering SDL.
        time.sleep(0.120)
    finally:
        runtime.xdotool(["mouseup", button])


def close_window(runtime: PrivateRuntime, window: int) -> None:
    runtime.xdotool(["windowactivate", "--sync", window])
    runtime.xdotool(["key", "--clearmodifiers", "alt+F4"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=REPOSITORY / "build/dev/moorhuhn2")
    args = parser.parse_args()
    binary = args.binary.resolve(strict=True)
    with PrivateRuntime(REPOSITORY, "startup", screen=(1280, 1024), window_manager=True) as runtime:
        result = {"status": "failed"}
        try:
            foreign = runtime.case_dir / "work" / "foreign-cwd"
            foreign.mkdir()
            rejected = runtime.launch_app([binary, "--unsupported"], timeout=5, cwd=foreign)
            if (runtime.wait(rejected) != 1
                    or "command-line arguments are not supported" not in rejected.log_path.read_text()):
                raise RuntimeError("The game accepted a command-line argument")
            app = runtime.launch_app([binary], timeout=45, cwd=foreign)
            window = runtime.wait_for_window(pid=app.pid, timeout=10)
            center_pointer(runtime, window)
            wait_title(runtime, app, window)
            size = geometry(runtime, window)
            if (size["WIDTH"], size["HEIGHT"]) != runtime.screen:
                raise RuntimeError("Startup did not fill the private screen")
            runtime.xdotool(["key", "F11"])
            time.sleep(.15)
            wait_title(runtime, app, window, "title-after-f11")
            if geometry(runtime, window) != size:
                raise RuntimeError("F11 changed the fullscreen window")
            close_window(runtime, window)
            assert_closed(runtime, app)
            defaults = (REPOSITORY / "tests/fixtures/behavior/ui/original-default.sav").read_bytes()
            if save_path(runtime).read_bytes() != defaults:
                raise RuntimeError("Normal shutdown did not save defaults in private storage")
            result["status"] = "passed"
        except BaseException as error:
            result["error"] = repr(error)
            raise
        finally:
            (runtime.case_dir / "artifacts/result.json").write_text(json.dumps(result, indent=2) + "\n")
            print(f"startup logs: {runtime.case_dir}", flush=True)


if __name__ == "__main__":
    main()

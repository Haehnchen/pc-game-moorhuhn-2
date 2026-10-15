#!/usr/bin/env python3
"""Check production startup, title pixels and private save storage without app instrumentation."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
import time

REPOSITORY = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY))

from tools.private_runtime import PrivateRuntime
from tools.pack_assets import read_manifest
from tools.image_pixels import RgbImage


def rgb_pixels(image: RgbImage):
    data = image.pixels
    return zip(data[0::3], data[1::3], data[2::3], strict=True)


def rgb565(image: RgbImage) -> RgbImage:
    return RgbImage(image.size, bytes(value for r, g, b in rgb_pixels(image)
                                    for value in (((r >> 3) * 255 + 15) // 31,
                                                  ((g >> 2) * 255 + 31) // 63,
                                                  ((b >> 3) * 255 + 15) // 31)))


def draw_text(image: RgbImage, font: RgbImage, text: str, x: int, y: int) -> RgbImage:
    pixels = bytearray(image.pixels)
    width, height = image.size
    font_width, _ = font.size
    for index, character in enumerate(text):
        if character == " ":
            continue
        glyph = (ord(character) - ord('A') + 31 if character.isupper()
                 else ord(character) - ord('a') + 57 if character.islower()
                 else ord(character) - ord('0') + 98)
        for row in range(50):
            for column in range(30):
                source = ((glyph * 50 + row) * font_width + column) * 3
                color = font.pixels[source:source + 3]
                left, top = x + index * 20 + column, y + row
                if color != b"\xff\x00\xff" and 0 <= left < width and 0 <= top < height:
                    target = (top * width + left) * 3
                    pixels[target:target + 3] = color
    return RgbImage(image.size, bytes(pixels))


def source_title(manifest: Path) -> tuple[RgbImage, dict]:
    document = read_manifest(manifest)
    matches = [entry for entry in document["images"] if entry["id"] == "gfx/mainscreen"]
    if len(matches) != 1:
        raise RuntimeError("Expected exactly one main-screen asset")
    entry = matches[0]
    title_path = (manifest.parent / entry["png"]["path"]).resolve(strict=True)
    if not title_path.is_relative_to(manifest.parent.resolve()):
        raise RuntimeError("Title image escapes the asset root")
    image = rgb565(RgbImage.load(title_path))
    if image.size != (640, 480):
        raise RuntimeError("Expected the supplied MH2 640x480 main screen")
    # The restored Extras route is an intentional title addition. Compose its
    # source font independently; keep all other background pixels exact.
    font_entry = next(entry for entry in document["images"] if entry["id"] == "gfx/font")
    font_path = manifest.parent / font_entry["png"]["path"]
    font = rgb565(RgbImage.load(font_path))
    image = draw_text(image, font, "Extras", 480, 375)
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
    return rgb565(RgbImage.load(REPOSITORY / "assets" / entry["png"]["path"]))


def scaled_screen(image: RgbImage, size: tuple[int, int], scale: int, x: int, y: int) -> RgbImage:
    width, height = size
    source_width, source_height = image.size
    pixels = bytearray(width * height * 3)
    for row in range(source_height):
        source = image.pixels[row * source_width * 3:(row + 1) * source_width * 3]
        expanded = b"".join(source[column:column + 3] * scale for column in range(0, len(source), 3))
        for repeat in range(scale):
            start = ((y + row * scale + repeat) * width + x) * 3
            pixels[start:start + len(expanded)] = expanded
    return RgbImage(size, bytes(pixels))


def screen_difference(actual: RgbImage, expected: RgbImage, masks, region) -> RgbImage:
    width, height = actual.size
    pixels = bytearray(abs(left - right) for left, right in zip(actual.pixels, expected.pixels, strict=True))
    for left, top, right, bottom in masks:
        for row in range(max(0, top), min(height, bottom)):
            start, end = (row * width + max(0, left)) * 3, (row * width + min(width, right)) * 3
            pixels[start:end] = bytes(end - start)
    if region:
        left, top, right, bottom = region
        return RgbImage((right - left, bottom - top), b"".join(
            pixels[(row * width + left) * 3:(row * width + right) * 3] for row in range(top, bottom)))
    return RgbImage(actual.size, bytes(pixels))


def wait_screen(runtime, app, window, expected, name, *, masks=(), region=None, timeout=30):
    """Observe ordinary window pixels; preserve the last mismatch for inspection."""
    size = geometry(runtime, window)
    width, height = size["WIDTH"], size["HEIGHT"]
    scale = min(width // 640, height // 480)
    if scale < 1:
        raise RuntimeError("Native screen checks require at least 640x480")
    x, y = (width - 640 * scale) // 2, (height - 480 * scale) // 2
    oracle = scaled_screen(expected, (width, height), scale, x, y)

    def rectangle(box):
        left, top, right, bottom = box
        return x + left * scale, y + top * scale, x + right * scale, y + bottom * scale

    masks = tuple(map(rectangle, masks))
    region = rectangle(region) if region else None

    def matches():
        actual = runtime.screenshot(f"artifacts/{name}.png", window=window)
        if actual.size != oracle.size:
            return False
        difference = screen_difference(actual, oracle, masks, region)
        if not any(difference.pixels):
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

"""RGB buffers for native checks; ImageMagick handles image file formats."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import subprocess


def _convert(arguments: list[str], *, pixels: bytes | None = None) -> bytes:
    return subprocess.run(
        ["convert", "-limit", "thread", "1", *arguments], input=pixels,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, timeout=10,
    ).stdout


@dataclass(frozen=True)
class RgbImage:
    size: tuple[int, int]
    pixels: bytes

    def __post_init__(self) -> None:
        width, height = self.size
        if width <= 0 or height <= 0 or len(self.pixels) != width * height * 3:
            raise ValueError("RGB buffer does not match its dimensions")

    @classmethod
    def load(cls, path: Path | str) -> RgbImage:
        # ImageMagick prints dimensions before the raw RGB stream, avoiding a
        # second decoder invocation and any Python image-format dependency.
        header, pixels = _convert([
            str(path), "-alpha", "off", "-depth", "8", "-print", "%w %h\n", "RGB:-",
        ]).split(b"\n", 1)
        width, height = map(int, header.split())
        return cls((width, height), pixels)

    def pixel(self, position: tuple[int, int]) -> tuple[int, int, int]:
        x, y = position
        width, height = self.size
        if not 0 <= x < width or not 0 <= y < height:
            raise ValueError("Pixel is outside the image")
        offset = (y * width + x) * 3
        return tuple(self.pixels[offset:offset + 3])

    def save(self, path: Path | str) -> None:
        width, height = self.size
        _convert(["-size", f"{width}x{height}", "-depth", "8", "RGB:-", str(path)], pixels=self.pixels)

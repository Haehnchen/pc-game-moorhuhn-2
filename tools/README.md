# Build and test tools

- `pack_assets.py`: packs the committed `assets/` into `images.pak` and
  `audio.pak` during the normal build. `assets/manifest.txt` contains image sizes,
  sprite grids and audio metadata.
- `packaging/package_release.py`: creates the ZIPs for `make release`;
  see [packaging](packaging/README.md).
- `private_runtime.py`: provides private displays, storage, silent audio and
  process cleanup for [native checks](../tests/integration/README.md).

All Python tools use only the standard library. Native image checks use
ImageMagick to read and write images through `image_pixels.py`.

```sh
python3 -B -m unittest discover -s tests/tools -p 'test_*.py' -v
```

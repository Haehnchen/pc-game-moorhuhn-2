# Build and test tools

- `pack_assets.py`: packs the committed `assets/` into `images.pak` and
  `audio.pak` during the normal build. `assets/manifest.txt` contains image sizes,
  sprite grids and audio metadata.
- `packaging/package_release.py`: creates the ZIPs for `make release`;
  see [packaging](packaging/README.md).
- `private_runtime.py`: provides private displays, storage, silent audio and
  process cleanup for [native checks](../tests/integration/README.md).

The build and packaging tools use only the Python standard library.

```sh
python3 -B -m unittest discover -s tests/tools -p 'test_*.py' -v
```

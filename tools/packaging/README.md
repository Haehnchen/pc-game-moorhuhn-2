# Release packages

Run from a clean checkout. On Windows, use the MSYS2 UCRT64 shell.

```sh
make release
```

ZIPs are written to `dist/` for the current platform:

- Linux: `moorhuhn2-Linux-x86_64.zip` and `moorhuhn2-Linux-x86_64-bundled.zip`.
- Windows/MSYS2 UCRT64: `moorhuhn2-Windows-x86_64-bundled.zip`.
- macOS: `moorhuhn2-macOS-arm64-bundled.zip` or `moorhuhn2-macOS-x86_64-bundled.zip`.

To package local changes:

```sh
make release PACKAGE_ARGS=--allow-dirty
```

`package_release.py` uses the Linux ELF, dependency and asset checks from `linux_package.py`.

CI and local builds share the ZIP completeness and CRC checks:

```sh
python3 -B tools/packaging/verify_archives.py --platform Linux --arch x86_64
```

Select `Windows` or `macOS` and the matching architecture for other builds.
Without filters, the verifier requires all five release archives. Use
`--output-dir verified` to copy that complete set only after every archive passes.

Packaging tests:

```sh
python3 -B -m unittest discover -s tests/tools -p 'test_*package*.py' -v
python3 -B tests/tools/test_verify_archives.py -v
```

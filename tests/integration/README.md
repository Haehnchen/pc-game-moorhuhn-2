# Native regression checks

These scripts run the normal executable on owned private displays with
private preferences and silent audio. They inspect window pixels, exit status and
saved files; the game has no diagnostic mode or command-line arguments.
The checks require Xvfb, xdotool, xdpyinfo,
openbox, xprop, ImageMagick and Pillow. Cases and failure logs remain under
`.local/private-runtime/` after owned processes and display endpoints are cleaned.

```sh
python3 -B tests/integration/round_native.py --binary build/dev/moorhuhn2
python3 -B tests/integration/whole_game_native.py --binary build/dev/moorhuhn2
```

Startup covers fullscreen title pixels, letterboxing, foreign working directories,
argument rejection and private save storage. The short lifecycle covers the
disclaimer, round entry, B-screen, focus recovery, results, title return and
save/restart. It ends the round with Escape. Internal event order, clock values,
ammunition and score calculations are covered by the C++ module tests, without
reading state from application logs.

```sh
python3 -B tests/integration/wayland_startup.py --binary build/dev/moorhuhn2
```

The Wayland check covers first presentation without injected focus and focus
recovery; it needs bubblewrap and Weston 14 under `.local/tools/weston/root`
(or `--weston-root`).
Native regression is run separately from unit tests and resource-heavy builds.

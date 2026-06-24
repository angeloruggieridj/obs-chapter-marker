# OBS Chapter Marker

A native, cross-platform [OBS Studio](https://obsproject.com/) plugin that gives
**recording chapter markers** the interface they're missing.

OBS has supported chapter markers since 30.2, but only through a hotkey buried in
Settings — there is **no button and no on-screen confirmation** when a marker is
added. This plugin fixes that, mirroring the replay-buffer experience (a button
plus a bottom-left status message confirming the action):

- 🎬 **Add Chapter Marker** button in a dockable panel.
- 🏷️ **Custom or auto names** (`Chapter 1`, `Chapter 2`, …).
- 📜 **Session log** of every marker — name, recording timestamp, wall-clock time.
- 🔴 **Recording-state aware** — the button enables only while recording.
- ✅ **Visible confirmation** — a message in OBS's bottom-left status bar *and* a
  status line inside the dock.
- ⌨️ **Own hotkey** — bindable in *Settings → Hotkeys* (“Add Chapter Marker”).
- 📤 **Export** the session markers to formats each editor imports natively:
  **YouTube** chapters (`.txt`), **DaVinci Resolve** (`.edl`), **Premiere Pro**
  (`.csv`) and **Final Cut Pro** (`.fcpxml`).

> The plugin triggers OBS's built-in chapter API, so markers land in your
> recording exactly as if you had used the native hotkey — you just finally get
> feedback that it worked.

## Requirements

- OBS Studio **30.2 or newer** (CI builds against 32.1.2; minimum verified 30.2).
- Recording to a container that supports chapters (e.g. **Hybrid MP4** or
  **MKV**). With a format that can't store chapters, the dock shows an error
  instead of silently doing nothing.

## Installation

Download the latest archive for your platform from the
[Releases](https://github.com/angeloruggieridj/obs-chapter-marker/releases) page.

| Platform | Install location |
| --- | --- |
| **Windows** | Extract into `%APPDATA%\obs-studio\plugins\` (so you get `…\plugins\obs-chapter-marker\bin\64bit\…`). |
| **Linux** | Extract the tarball into `/usr` (system) or `~/.config/obs-studio/plugins/`. |
| **macOS** | Copy `obs-chapter-marker.plugin` into `~/Library/Application Support/obs-studio/plugins/`. |

Restart OBS, then enable the dock from **Docks → Chapter Marker**.

## Usage

1. Start a recording (chapters only apply to recordings).
2. Open the **Chapter Marker** dock.
3. Optionally type a name; otherwise the next auto name is used.
4. Click **Add Chapter Marker** (or press your hotkey). The marker is written to
   the recording and confirmed in the status bar and the dock log.

Bind a hotkey under **Settings → Hotkeys → Add Chapter Marker** for hands-free
marking.

### Exporting markers

Click **Export…** and choose a target:

| Target | File | Import in the editor |
| --- | --- | --- |
| YouTube | `.txt` | Paste the lines into the video description. |
| DaVinci Resolve | `.edl` | Timeline → *Import → Timeline Markers from EDL*. |
| Premiere Pro | `.csv` | Markers panel → *Import Markers from CSV*. |
| Final Cut Pro | `.fcpxml` | *File → Import → XML*. |

Timecodes use the current OBS output frame rate. Drop-frame rates (29.97, 59.94)
are written as non-drop in this version, so timecodes may differ by a few frames
from a drop-frame timeline over long recordings.

## Building from source

```sh
# Unit tests only (no OBS/Qt needed):
cmake -B build -S . -DBUILD_PLUGIN=OFF -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure

# Full plugin (needs libobs, obs-frontend-api and Qt 6):
cmake -B build -S . -DCMAKE_PREFIX_PATH="/path/to/obs-dev;/path/to/qt6"
cmake --build build
cmake --install build --prefix /your/obs/plugin/prefix
```

The build is split so the pure logic (`src/core`) compiles and is unit-tested
without an OBS runtime; the OBS/Qt glue lives in `src/plugin`. See
[`.github/workflows/build_project.yml`](.github/workflows/build_project.yml) for
exact per-platform invocations.

## Project layout

```
src/core      Pure, unit-tested logic (naming, recording clock, formatting)
src/plugin    OBS + Qt: dock widget, hotkey, frontend events
tests         doctest suites over src/core
data/locale   Translations (10 languages)
docs          Design spec and implementation plan
```

## Localization

Shipped in 10 languages: English (`en-US`), Italian (`it-IT`), German (`de-DE`),
Spanish (`es-ES`), French (`fr-FR`), Japanese (`ja-JP`), Korean (`ko-KR`),
Brazilian Portuguese (`pt-BR`), Russian (`ru-RU`) and Simplified Chinese
(`zh-CN`). To add a language, copy `data/locale/en-US.ini` to your locale code
and translate the values.

## License

[MIT](LICENSE) © 2026 Angelo Ruggieri.

---

Sibling project: [obs-playlist-deck](https://github.com/angeloruggieridj/obs-playlist-deck)
— drive an OBS media source from a native playlist dock.

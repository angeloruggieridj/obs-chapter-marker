# obs-chapter-marker — Design

**Date:** 2026-06-24
**Status:** Approved
**Author:** angeloruggieridj

## 1. Purpose & scope

Native, cross-platform OBS Studio plugin that adds the missing user interface for
OBS's built-in *recording chapter markers*. OBS has supported chapter markers
since 30.2 (via `obs_frontend_recording_add_chapter()` and a hotkey), but the
stock UI gives **no button and no on-screen feedback** when a marker is added.

This plugin fixes that, mirroring the UX of the replay buffer (where a "Save
Replay" button appears and a bottom-left status message confirms the save):

- A dockable panel with an **Add Chapter Marker** button.
- Custom / auto-incrementing marker names.
- A **session log** of markers added (name + recording elapsed + wall clock).
- **Recording-state awareness** (button disabled when not recording).
- Visible confirmation: OBS main-window **status-bar message** (bottom-left,
  like the replay-saved toast) **plus** an in-dock status line.

The plugin **owns its trigger**: both the dock button and a plugin-registered
hotkey call `obs_frontend_recording_add_chapter()`, so the plugin always knows
when a marker fired and can log/confirm it reliably. (OBS emits no event when
the *core* hotkey adds a chapter, so that path cannot be observed by a plugin.)

**Out of scope for v1:** Stream Deck companion (the existing obs-playlist-deck
ships one; here the hotkey already covers fast triggering — add later).

## 2. Architecture

Mirrors the obs-playlist-deck repository: C++17, CMake ≥ 3.22, Qt 6, OBS 31.0+
(API needs exist since OBS 30.2), MIT license, GitHub Actions CI building
Windows x64 / Linux x86_64 / macOS universal.

```
obs-chapter-marker/
├── .github/workflows/build_project.yml   # CI: tests + 3-platform build + release
├── CMakeLists.txt                        # top-level: core + tests + plugin
├── LICENSE                               # MIT
├── README.md
├── data/locale/{en-US,it-IT}.ini         # i18n
├── docs/superpowers/{specs,plans}/
├── third_party/doctest/doctest.h         # unit-test framework
├── src/
│   ├── core/        # pure logic, no Qt/libobs — unit tested
│   │   ├── ChapterName.{hpp,cpp}    # naming policy
│   │   ├── RecordingClock.{hpp,cpp} # elapsed time with pause handling
│   │   ├── TimeFormat.{hpp,cpp}     # ms -> HH:MM:SS
│   │   └── Version.{hpp,cpp}        # version compare (update check)
│   └── plugin/      # Qt + libobs
│       ├── plugin-main.cpp          # module entry, dock + frontend events
│       └── ChapterMarkerDock.{hpp,cpp}
└── tests/           # doctest suites over src/core
```

### Separation of concerns

- **core** has no Qt and no libobs dependency, so every naming / timing rule is
  unit-testable on CI without an OBS runtime. It is built as a static,
  position-independent library and linked into both the plugin module and the
  test binary.
- **plugin** holds all OBS/Qt glue: dock widget, hotkey registration, frontend
  event handling, and the calls into `obs-frontend-api`.

## 3. Components

### core/ChapterName
`resolve(custom, counter)` → final marker name. Trims whitespace; if the custom
field is empty, returns the auto name `"<prefix> <counter>"` (default prefix
`"Chapter"`). Pure function — drives both button and hotkey paths identically.

### core/RecordingClock
Tracks recording elapsed time independent of OBS, given an injected "now"
(monotonic milliseconds): `start(now)`, `pause(now)`, `resume(now)`,
`stop()`, `elapsedMs(now)`. Accumulates paused time so the reported chapter
timestamp matches the recording timeline. Pure logic → fully unit-tested.

### core/TimeFormat
`hms(ms)` → `"HH:MM:SS"` (zero-padded, hours uncapped).

### core/Version
`isNewerVersion(latest, current)` — dotted version compare for the optional
GitHub update check. (Same contract as obs-playlist-deck's.)

### plugin/plugin-main.cpp
`OBS_DECLARE_MODULE`, default locale `en-US`. On
`OBS_FRONTEND_EVENT_FINISHED_LOADING` it constructs the dock and registers it
with `obs_frontend_add_custom_qdock`. Routes recording frontend events
(`RECORDING_STARTED/STOPPED/PAUSED/UNPAUSED`) to the dock. Cleans up on
`OBS_FRONTEND_EVENT_EXIT`.

### plugin/ChapterMarkerDock
`QDockWidget` containing: marker-name `QLineEdit` (placeholder shows the next
auto name), **Add Chapter Marker** `QPushButton`, a `QListWidget` session log, a
bottom status `QLabel`, and a version label. Owns the `RecordingClock` and the
auto counter. Registers one frontend hotkey ("Add Chapter Marker"). Reacts to
recording-state changes to enable/disable the button.

## 4. Data flow (adding a marker)

1. User clicks the button or presses the plugin hotkey.
2. Dock resolves the name via `ChapterName::resolve(field, counter)`.
3. Dock calls `obs_frontend_recording_add_chapter(name)`.
4. **On success:** increment counter; append a log row
   `"<name> — <elapsed HH:MM:SS> (<wall clock>)"`; set the dock status line green
   `✓`; push a 4 s message to the OBS main-window status bar
   (`obs_frontend_get_main_window()` → `QMainWindow::statusBar()->showMessage`).
5. **On failure:** red status line + status-bar warning, no log entry.

## 5. Recording-state awareness

Frontend events drive a single `setRecordingState(active, paused)`:

- not recording → button disabled, status line "Recording not active".
- recording started → `RecordingClock::start`, counter reset (per-recording),
  button enabled.
- paused / unpaused → `RecordingClock::pause` / `resume`.
- stopped → `RecordingClock::stop`, button disabled.

## 6. Error handling

- `obs_frontend_recording_add_chapter` returns `false` (e.g. the recording
  format/container does not support chapters, like `.mp4` vs `.mkv`/`.hybrid`):
  surfaced as a red status line + status-bar warning; no log entry.
- `obs_frontend_get_main_window()` null / not a `QMainWindow`: fall back to the
  dock status line only; never dereference null.
- Empty name field → auto `Chapter N` (never sends an empty name).

## 7. Testing

- **Unit (CI, headless):** doctest suites over `core` — `ChapterName`
  (trim, empty→auto, counter, custom prefix), `RecordingClock` (start/elapsed,
  pause accumulation, resume, stop, multiple pauses), `TimeFormat` (padding,
  hours > 99, zero), `Version` (newer/older/equal, `v` prefix, missing parts).
- **Manual:** dock UI verified in a live OBS recording session (button, log,
  status bar, disabled-when-not-recording).
- CI builds all three platforms; no headless OBS UI test.

## 8. Repo-parity deliverables

README (features / install / build / usage), MIT LICENSE, GitHub Actions CI
(unit tests gate → per-platform build → tagged release), `data/locale` i18n
(en-US, it-IT), version/product metadata in CMake.

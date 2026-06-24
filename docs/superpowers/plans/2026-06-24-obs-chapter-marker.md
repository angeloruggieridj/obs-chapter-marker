# obs-chapter-marker — Implementation Plan

**Date:** 2026-06-24
**Spec:** [2026-06-24-obs-chapter-marker-design.md](../specs/2026-06-24-obs-chapter-marker-design.md)

## Goal

Ship a native cross-platform OBS plugin that adds a dock UI + confirmation for
OBS's built-in recording chapter markers, matching the obs-playlist-deck repo
conventions.

## Milestones

### 1. Repo scaffold (parity with obs-playlist-deck)
- Top-level `CMakeLists.txt` (core + tests + optional plugin).
- `.gitignore`, `.gitattributes`, MIT `LICENSE`.
- `third_party/doctest/doctest.h` vendored.
- GitHub Actions `build_project.yml`: unit-test gate → Linux/Windows/macOS
  builds → tagged release. Stream Deck and FFmpeg jobs removed (not needed).

### 2. Core logic (pure, unit-tested) — `src/core`
- `ChapterName` — `resolveChapterName(custom, counter, prefix)`: trim, empty →
  auto `Chapter N`.
- `RecordingClock` — elapsed ms with pause accumulation, monotonic "now"
  injected for testability.
- `TimeFormat` — `formatHms(ms)` → `HH:MM:SS`.
- `Version` — `isNewerVersion` for the optional update check.
- doctest suites for each.

### 3. Plugin (OBS + Qt) — `src/plugin`
- `plugin-main.cpp` — module entry; register dock via
  `obs_frontend_add_custom_qdock`; route recording frontend events to the dock.
- `ChapterMarkerDock` — name field, Add button, session log, status line,
  version label; owns `RecordingClock` + auto counter; registers the
  "Add Chapter Marker" frontend hotkey; calls
  `obs_frontend_recording_add_chapter`; confirms via OBS status bar +
  dock status line.

### 4. i18n + docs
- `data/locale/en-US.ini`, `data/locale/it-IT.ini`.
- README (features/install/build/usage).

### 5. Verify + publish
- Build and run core unit tests locally.
- Create the public GitHub repo and push; CI builds all platforms.

## Key decisions

- **Plugin owns the trigger** (button + own hotkey both call the OBS API): OBS
  emits no event for the core hotkey, so this is the only reliable way to log and
  confirm markers.
- **Build OBS-frontend-api from source in CI** on every platform: distro libobs
  predates `obs_frontend_recording_add_chapter` (30.2+).
- **Confirmation = status bar + dock status line** (approach A+C from the spec):
  native feel, low risk, no custom toast widget.

## Risks

- `obs_frontend_recording_add_chapter` returns false for containers without
  chapter support → surfaced as an error in the dock, not a crash.
- Status-bar access casts the OBS main window to `QMainWindow` → null-guarded,
  falls back to the dock status line.

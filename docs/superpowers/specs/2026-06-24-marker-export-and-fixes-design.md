# Marker Export + first-test fixes — Design

**Date:** 2026-06-24
**Status:** Approved
**Depends on:** [2026-06-24-obs-chapter-marker-design.md](2026-06-24-obs-chapter-marker-design.md)

Addresses three items from the first live test:

1. **Bug** — when recording starts, the dock status label stays on the green
   "Recording not active" text.
2. **Feature** — export the session markers to files each target NLE imports
   natively (YouTube, DaVinci Resolve, Premiere Pro, Final Cut Pro).
3. **Bug** — the chapter-added confirmation never appeared in the OBS bottom-left
   status bar (the area used for encoder overload, replay saved, disconnect…).

## 1. Bug #1 — stale / mis-coloured status label

**Cause.** `ChapterMarkerDock::updateButtonState()` only refreshes the status
text `else if (status_->text().isEmpty())`. After the dock first shows
"Recording not active", the text is non-empty, so the `RECORDING_STARTED`
transition never updates it. Separately, state messages are drawn green because
`setStatus(msg)` defaults to the success colour.

**Fix.** Replace the boolean `error` flag with a status *level*:

- `Neutral` — state messages (ready / not recording / paused): theme default
  colour (clear the stylesheet).
- `Success` — green (✓ chapter added).
- `Error` — red (add failed).

`updateButtonState()` always sets the current state message (no `isEmpty`
guard), so every recording-state transition refreshes the label. Transient
Success/Error messages from `onAddChapter` are shown until the next state change.

## 2. Bug #3 / item F — confirmation missing from OBS status bar

**Cause.** `showStatusBarMessage()` used `QMainWindow::statusBar()`, which
*creates and returns an empty status bar* if OBS's real status bar was not
registered via `setStatusBar()` — so the message went to a phantom, invisible
bar.

**Fix.** Target OBS's actual visible status bar, mirroring OBS's own
`OBSBasic::ShowStatusBarMessage` (`ui->statusbar->clearMessage();
ui->statusbar->showMessage(msg, 10000);`):

```cpp
auto* mw = static_cast<QMainWindow*>(obs_frontend_get_main_window());
QStatusBar* sb = mw ? mw->findChild<QStatusBar*>() : nullptr; // OBSBasicStatusBar
if (!sb && mw) sb = mw->statusBar();
if (sb) { sb->clearMessage(); sb->showMessage(msg, 10000); }
```

`findChild<QStatusBar*>()` returns OBS's `OBSBasicStatusBar` (it is a
`QStatusBar`), the visible one. Falls back to the dock status line if absent.
Message timeout aligned to OBS's 10 s.

## 3. Feature — native marker export

### Data model
The dock currently stores log rows as strings. Introduce a structured list:

```cpp
struct Marker { std::string name; int64_t elapsedMs; std::string wallClock; };
std::vector<Marker> markers_;
```

The `QListWidget` remains the view; export reads `markers_`. Cleared on a new
recording (same lifetime as the counter).

### Frame rate
Timecode-based formats need fps. The plugin reads `obs_get_video_info(&ovi)` and
passes `fps = ovi.fps_num / (double)ovi.fps_den` to the core. Drop-frame rates
(29.97, 59.94) are written as **non-drop** in v1 (documented caveat).

### Core exporters — `src/core/Timecode.{hpp,cpp}` + `src/core/Export.{hpp,cpp}`
Pure functions, no Qt/OBS, unit-tested. Input type:

```cpp
struct MarkerEntry { std::string name; int64_t ms; };
```

- `formatTimecode(int64_t ms, double fps)` → `"HH:MM:SS:FF"` (non-drop, frames
  floored, frame clamped to `fps-1`).
- `exportYouTube(entries)` → text, one `H:MM:SS Name` per line (hours omitted
  when zero, per YouTube convention). **If the first entry is not at 0 ms,
  prepend `0:00 Intro`** (YouTube requires the first chapter at 0:00).
- `exportEdl(entries, fps)` → CMX3600 EDL with one event per marker carrying a
  green timeline marker comment (`* LOC:` / colour) that DaVinci Resolve imports
  as a timeline marker. `TITLE:` header + `FCM: NON-DROP FRAME`.
- `exportPremiereCsv(entries, fps)` → CSV with header
  `Marker Name,Description,In,Out,Duration,Marker Type` and one comment marker
  per row (`In` = timecode, empty `Out`/`Duration`).
- `exportFcpxml(entries, fps)` → minimal FCPXML (v1.10) with a `<project>` whose
  spine holds a gap carrying `<marker start=… value=Name/>` elements, the form
  the current Final Cut Pro imports.

Each exporter returns a `std::string`; the plugin writes it to the chosen file.

### Dock UI
An **Export…** `QPushButton` next to/under the log. Clicking opens a `QMenu`:
*YouTube*, *DaVinci Resolve (EDL)*, *Premiere Pro (CSV)*, *Final Cut Pro
(FCPXML)*. The chosen entry opens `QFileDialog::getSaveFileName` with the right
default name/extension/filter, writes the file, and confirms via the dock status
line + OBS status bar. The button is disabled while `markers_` is empty.

### Error handling
- No markers → Export button disabled (can't reach the dialog).
- File write failure → red status line + status-bar warning.
- fps unavailable / zero → fall back to 30.0 and log a warning.

## 4. Testing

doctest suites over the core:
- `Timecode` — integer fps, 29.97→30 rounding behaviour, 0 ms, sub-frame floor,
  multi-hour.
- `Export` — YouTube line format + auto 0:00 prepend + hours omission; EDL header
  and event/marker lines; Premiere CSV header + a row; FCPXML well-formedness
  (contains expected `<marker>` values and escaped names). Empty-list handling
  for each.

Core stays free of Qt/OBS, so all of the above runs headless on CI. Dock wiring
(button, menu, file dialog) verified manually in OBS.

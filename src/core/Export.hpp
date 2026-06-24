#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ocm {

// One chapter marker for export: a name and its recording-relative timestamp.
struct MarkerEntry {
    std::string name;
    int64_t ms;
};

// YouTube video-description chapters: one "M:SS Name" (or "H:MM:SS Name") line
// per marker. YouTube requires the first chapter at 0:00, so a "0:00 Intro"
// line is prepended when the first marker is not at 0 ms.
std::string exportYouTube(const std::vector<MarkerEntry>& entries);

// CMX3600 EDL with one green timeline marker per entry — imported by DaVinci
// Resolve via "Import > Timeline Markers from EDL".
std::string exportEdl(const std::vector<MarkerEntry>& entries, double fps);

// Adobe Premiere Pro marker CSV (Marker Name,Description,In,Out,Duration,
// Marker Type).
std::string exportPremiereCsv(const std::vector<MarkerEntry>& entries, double fps);

// Minimal FCPXML (1.10) carrying the markers on a spine gap — imported by the
// current Final Cut Pro.
std::string exportFcpxml(const std::vector<MarkerEntry>& entries, double fps);

} // namespace ocm

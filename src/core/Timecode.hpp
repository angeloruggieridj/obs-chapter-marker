#pragma once
#include <cstdint>
#include <string>

namespace ocm {

// Formats a duration in milliseconds as an SMPTE-style non-drop timecode
// "HH:MM:SS:FF" at the given frame rate. Frames are floored from the
// sub-second remainder and clamped to (round(fps) - 1). fps <= 0 falls back to
// 30. Negative input clamps to zero.
std::string formatTimecode(int64_t ms, double fps);

} // namespace ocm

#pragma once
#include <cstdint>
#include <string>

namespace ocm {

// Formats a duration in milliseconds as "HH:MM:SS" (zero-padded; hours are not
// capped, so a 100-hour recording yields "100:00:00"). Negative input clamps
// to zero.
std::string formatHms(int64_t ms);

} // namespace ocm

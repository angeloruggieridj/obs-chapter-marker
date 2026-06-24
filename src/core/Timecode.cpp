#include "Timecode.hpp"
#include <cmath>
#include <cstdio>

namespace ocm {

std::string formatTimecode(int64_t ms, double fps) {
    if (ms < 0) ms = 0;
    if (!(fps > 0.0)) fps = 30.0;
    int rate = static_cast<int>(std::lround(fps));
    if (rate < 1) rate = 1;

    int64_t totalSec = ms / 1000;
    int64_t h = totalSec / 3600;
    int m = static_cast<int>((totalSec % 3600) / 60);
    int s = static_cast<int>(totalSec % 60);
    int frame = static_cast<int>((ms % 1000) * rate / 1000);
    if (frame > rate - 1) frame = rate - 1;

    char buf[40];
    std::snprintf(buf, sizeof(buf), "%02lld:%02d:%02d:%02d",
                  static_cast<long long>(h), m, s, frame);
    return std::string(buf);
}

} // namespace ocm

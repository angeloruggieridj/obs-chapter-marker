#include "TimeFormat.hpp"
#include <cstdio>

namespace ocm {

std::string formatHms(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t totalSec = ms / 1000;
    int64_t h = totalSec / 3600;
    int m = static_cast<int>((totalSec % 3600) / 60);
    int s = static_cast<int>(totalSec % 60);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld:%02d:%02d",
                  static_cast<long long>(h), m, s);
    return std::string(buf);
}

} // namespace ocm

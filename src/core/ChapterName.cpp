#include "ChapterName.hpp"
#include <cctype>

namespace ocm {

static std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string resolveChapterName(const std::string& custom, int counter,
                               const std::string& autoPrefix) {
    std::string name = trim(custom);
    if (!name.empty()) return name;
    return autoPrefix + " " + std::to_string(counter);
}

} // namespace ocm

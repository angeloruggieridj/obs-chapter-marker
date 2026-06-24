#include "Export.hpp"
#include "Timecode.hpp"

#include <cmath>
#include <cstdio>
#include <sstream>

namespace ocm {

namespace {

int frameRate(double fps) {
    if (!(fps > 0.0)) fps = 30.0;
    int r = static_cast<int>(std::lround(fps));
    return r < 1 ? 1 : r;
}

// Frame index on the timeline for a timestamp (rounded to nearest frame).
int64_t frameIndex(int64_t ms, int rate) {
    if (ms < 0) ms = 0;
    return std::llround(static_cast<double>(ms) * rate / 1000.0);
}

// YouTube timestamp: "M:SS" (hours omitted when zero) or "H:MM:SS".
std::string youTubeStamp(int64_t ms) {
    if (ms < 0) ms = 0;
    int64_t totalSec = ms / 1000;
    int64_t h = totalSec / 3600;
    int m = static_cast<int>((totalSec % 3600) / 60);
    int s = static_cast<int>(totalSec % 60);
    char buf[32];
    if (h > 0)
        std::snprintf(buf, sizeof(buf), "%lld:%02d:%02d",
                      static_cast<long long>(h), m, s);
    else
        std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return std::string(buf);
}

std::string xmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out += c; break;
        }
    }
    return out;
}

std::string csvField(const std::string& s) {
    bool quote = s.find_first_of(",\"\n\r") != std::string::npos;
    if (!quote) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

} // namespace

std::string exportYouTube(const std::vector<MarkerEntry>& entries) {
    std::ostringstream os;
    bool needIntro = entries.empty() || entries.front().ms > 0;
    if (needIntro) os << "0:00 Intro\n";
    for (const auto& e : entries) {
        os << youTubeStamp(e.ms) << ' ' << e.name << '\n';
    }
    return os.str();
}

std::string exportEdl(const std::vector<MarkerEntry>& entries, double fps) {
    int rate = frameRate(fps);
    int64_t frameMs = 1000 / rate; // one-frame duration for the event out point
    std::ostringstream os;
    os << "TITLE: OBS Chapter Markers\r\n";
    os << "FCM: NON-DROP FRAME\r\n";
    int n = 1;
    for (const auto& e : entries) {
        std::string in = formatTimecode(e.ms, fps);
        std::string out = formatTimecode(e.ms + frameMs, fps);
        char ev[8];
        std::snprintf(ev, sizeof(ev), "%03d", n++);
        os << "\r\n"
           << ev << "  001      V     C        "
           << in << ' ' << out << ' ' << in << ' ' << out << "\r\n";
        os << " * LOC: " << in << " GREEN  " << e.name << "\r\n";
    }
    return os.str();
}

std::string exportPremiereCsv(const std::vector<MarkerEntry>& entries, double fps) {
    std::ostringstream os;
    os << "Marker Name,Description,In,Out,Duration,Marker Type\r\n";
    for (const auto& e : entries) {
        std::string in = formatTimecode(e.ms, fps);
        os << csvField(e.name) << ",," << in << ",,," << "Comment\r\n";
    }
    return os.str();
}

std::string exportFcpxml(const std::vector<MarkerEntry>& entries, double fps) {
    int rate = frameRate(fps);
    std::ostringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<!DOCTYPE fcpxml>\n";
    os << "<fcpxml version=\"1.10\">\n";
    os << "  <resources>\n";
    os << "    <format id=\"r1\" name=\"FFVideoFormat1080p" << rate
       << "\" frameDuration=\"1/" << rate
       << "s\" width=\"1920\" height=\"1080\"/>\n";
    os << "  </resources>\n";
    os << "  <library>\n";
    os << "    <event name=\"OBS Chapter Markers\">\n";
    os << "      <project name=\"Chapters\">\n";
    os << "        <sequence format=\"r1\">\n";
    os << "          <spine>\n";
    os << "            <gap name=\"Gap\" offset=\"0s\" duration=\"3600s\" start=\"0s\">\n";
    for (const auto& e : entries) {
        int64_t f = frameIndex(e.ms, rate);
        os << "              <marker start=\"" << f << '/' << rate
           << "s\" duration=\"1/" << rate << "s\" value=\""
           << xmlEscape(e.name) << "\"/>\n";
    }
    os << "            </gap>\n";
    os << "          </spine>\n";
    os << "        </sequence>\n";
    os << "      </project>\n";
    os << "    </event>\n";
    os << "  </library>\n";
    os << "</fcpxml>\n";
    return os.str();
}

} // namespace ocm

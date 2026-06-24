#include "doctest/doctest.h"
#include "Export.hpp"

using ocm::MarkerEntry;
using ocm::exportYouTube;
using ocm::exportEdl;
using ocm::exportPremiereCsv;
using ocm::exportFcpxml;

static std::vector<MarkerEntry> sample() {
    return {{"Intro", 0}, {"Demo", 135000}, {"Q&A", 460000}};
}

static bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

TEST_CASE("YouTube: one line per marker, M:SS format") {
    std::string out = exportYouTube(sample());
    CHECK(contains(out, "0:00 Intro\n"));
    CHECK(contains(out, "2:15 Demo\n"));
    CHECK(contains(out, "7:40 Q&A\n"));
}

TEST_CASE("YouTube: prepends 0:00 when first marker is not at zero") {
    std::vector<MarkerEntry> e = {{"Later", 5000}};
    std::string out = exportYouTube(e);
    CHECK(contains(out, "0:00 Intro\n"));
    CHECK(contains(out, "0:05 Later\n"));
}

TEST_CASE("YouTube: hours shown when over an hour") {
    std::vector<MarkerEntry> e = {{"Start", 0}, {"Mark", 3725000}}; // 1:02:05
    std::string out = exportYouTube(e);
    CHECK(contains(out, "1:02:05 Mark"));
}

TEST_CASE("YouTube: empty list still yields an intro") {
    CHECK(exportYouTube({}) == "0:00 Intro\n");
}

TEST_CASE("EDL: header and a LOC marker line") {
    std::string out = exportEdl(sample(), 30.0);
    CHECK(contains(out, "TITLE: OBS Chapter Markers"));
    CHECK(contains(out, "FCM: NON-DROP FRAME"));
    CHECK(contains(out, "* LOC: 00:02:15:00 GREEN  Demo"));
    CHECK(contains(out, "001  001      V     C"));
}

TEST_CASE("Premiere CSV: header and a comment row") {
    std::string out = exportPremiereCsv(sample(), 30.0);
    CHECK(contains(out, "Marker Name,Description,In,Out,Duration,Marker Type"));
    CHECK(contains(out, "Demo,,00:02:15:00,,,Comment"));
}

TEST_CASE("Premiere CSV: quotes fields with commas") {
    std::vector<MarkerEntry> e = {{"Hello, world", 0}};
    std::string out = exportPremiereCsv(e, 30.0);
    CHECK(contains(out, "\"Hello, world\",,00:00:00:00,,,Comment"));
}

TEST_CASE("FCPXML: well-formed-ish with escaped marker values") {
    std::string out = exportFcpxml(sample(), 30.0);
    CHECK(contains(out, "<fcpxml version=\"1.10\">"));
    CHECK(contains(out, "frameDuration=\"1/30s\""));
    CHECK(contains(out, "value=\"Demo\""));
    CHECK(contains(out, "value=\"Q&amp;A\"")); // & escaped
    // 135000 ms @ 30 fps = 4050 frames
    CHECK(contains(out, "start=\"4050/30s\""));
}

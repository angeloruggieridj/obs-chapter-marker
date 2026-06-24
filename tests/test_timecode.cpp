#include "doctest/doctest.h"
#include "Timecode.hpp"

using ocm::formatTimecode;

TEST_CASE("zero") {
    CHECK(formatTimecode(0, 30.0) == "00:00:00:00");
}

TEST_CASE("whole seconds, no frames") {
    CHECK(formatTimecode(5000, 30.0) == "00:00:05:00");
    CHECK(formatTimecode(3661000, 25.0) == "01:01:01:00");
}

TEST_CASE("sub-second becomes frames") {
    // 500 ms at 30 fps = 15 frames
    CHECK(formatTimecode(500, 30.0) == "00:00:00:15");
    // 999 ms at 30 fps -> floor(29.97) = 29 frames, clamped to 29
    CHECK(formatTimecode(999, 30.0) == "00:00:00:29");
}

TEST_CASE("29.97 rounds to 30 for frame count") {
    CHECK(formatTimecode(0, 29.97) == "00:00:00:00");
    CHECK(formatTimecode(500, 29.97) == "00:00:00:15");
}

TEST_CASE("frame clamped to rate-1") {
    // 24 fps: 999 ms -> floor(23.976)=23, clamp keeps 23
    CHECK(formatTimecode(999, 24.0) == "00:00:00:23");
}

TEST_CASE("invalid fps falls back to 30") {
    CHECK(formatTimecode(500, 0.0) == "00:00:00:15");
    CHECK(formatTimecode(500, -5.0) == "00:00:00:15");
}

TEST_CASE("negative clamps to zero") {
    CHECK(formatTimecode(-1, 30.0) == "00:00:00:00");
}

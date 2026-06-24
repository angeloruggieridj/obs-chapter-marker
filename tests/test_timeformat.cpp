#include "doctest/doctest.h"
#include "TimeFormat.hpp"

using ocm::formatHms;

TEST_CASE("zero") {
    CHECK(formatHms(0) == "00:00:00");
}

TEST_CASE("seconds and minutes") {
    CHECK(formatHms(5000) == "00:00:05");
    CHECK(formatHms(65000) == "00:01:05");
    CHECK(formatHms(3661000) == "01:01:01");
}

TEST_CASE("truncates sub-second") {
    CHECK(formatHms(1999) == "00:00:01");
}

TEST_CASE("hours not capped") {
    CHECK(formatHms(360000000) == "100:00:00");
}

TEST_CASE("negative clamps to zero") {
    CHECK(formatHms(-1) == "00:00:00");
}

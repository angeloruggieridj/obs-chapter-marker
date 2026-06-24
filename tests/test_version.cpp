#include "doctest/doctest.h"
#include "Version.hpp"

using ocm::isNewerVersion;

TEST_CASE("strictly newer") {
    CHECK(isNewerVersion("1.0.1", "1.0.0"));
    CHECK(isNewerVersion("1.1.0", "1.0.9"));
    CHECK(isNewerVersion("2.0.0", "1.9.9"));
}

TEST_CASE("not newer when equal or older") {
    CHECK_FALSE(isNewerVersion("1.0.0", "1.0.0"));
    CHECK_FALSE(isNewerVersion("1.0.0", "1.0.1"));
}

TEST_CASE("leading v ignored") {
    CHECK(isNewerVersion("v1.2.0", "1.1.0"));
    CHECK_FALSE(isNewerVersion("v1.0.0", "v1.0.0"));
}

TEST_CASE("missing components treated as zero") {
    CHECK_FALSE(isNewerVersion("1.2", "1.2.0"));
    CHECK(isNewerVersion("1.2.1", "1.2"));
}

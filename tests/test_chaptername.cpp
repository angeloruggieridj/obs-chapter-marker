#include "doctest/doctest.h"
#include "ChapterName.hpp"

using ocm::resolveChapterName;

TEST_CASE("custom name used verbatim when non-empty") {
    CHECK(resolveChapterName("Intro", 1) == "Intro");
    CHECK(resolveChapterName("Goal!", 7) == "Goal!");
}

TEST_CASE("empty custom falls back to auto name with counter") {
    CHECK(resolveChapterName("", 1) == "Chapter 1");
    CHECK(resolveChapterName("", 42) == "Chapter 42");
}

TEST_CASE("whitespace-only custom is treated as empty") {
    CHECK(resolveChapterName("   ", 3) == "Chapter 3");
    CHECK(resolveChapterName("\t\n ", 5) == "Chapter 5");
}

TEST_CASE("custom name is trimmed") {
    CHECK(resolveChapterName("  Highlight  ", 2) == "Highlight");
}

TEST_CASE("custom auto prefix") {
    CHECK(resolveChapterName("", 4, "Marker") == "Marker 4");
}

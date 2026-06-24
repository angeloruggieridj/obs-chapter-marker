#include "doctest/doctest.h"
#include "RecordingClock.hpp"

using ocm::RecordingClock;

TEST_CASE("not running reports zero") {
    RecordingClock c;
    CHECK_FALSE(c.running());
    CHECK(c.elapsedMs(1000) == 0);
}

TEST_CASE("elapsed counts from start") {
    RecordingClock c;
    c.start(1000);
    CHECK(c.running());
    CHECK(c.elapsedMs(1000) == 0);
    CHECK(c.elapsedMs(6000) == 5000);
}

TEST_CASE("pause excludes paused span") {
    RecordingClock c;
    c.start(0);
    c.pause(5000);          // 5s recorded
    CHECK(c.paused());
    CHECK(c.elapsedMs(8000) == 5000); // frozen while paused
    c.resume(8000);         // 3s of pause skipped
    CHECK_FALSE(c.paused());
    CHECK(c.elapsedMs(10000) == 7000); // 5s + 2s
}

TEST_CASE("multiple pauses accumulate") {
    RecordingClock c;
    c.start(0);
    c.pause(2000);
    c.resume(4000);   // skipped 2s
    c.pause(6000);    // recorded 4s total of wall 6s
    c.resume(9000);   // skipped another 3s
    CHECK(c.elapsedMs(10000) == 5000); // 10s wall - 5s paused
}

TEST_CASE("stop resets") {
    RecordingClock c;
    c.start(0);
    c.elapsedMs(5000);
    c.stop();
    CHECK_FALSE(c.running());
    CHECK(c.elapsedMs(9000) == 0);
}

TEST_CASE("redundant pause/resume are no-ops") {
    RecordingClock c;
    c.start(0);
    c.resume(1000);   // not paused -> ignored
    CHECK(c.elapsedMs(2000) == 2000);
    c.pause(2000);
    c.pause(3000);    // already paused -> ignored
    c.resume(4000);   // skips 2s (2000..4000)
    CHECK(c.elapsedMs(5000) == 3000); // 5s - 2s paused
}

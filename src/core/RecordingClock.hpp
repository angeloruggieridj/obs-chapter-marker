#pragma once
#include <cstdint>

namespace ocm {

// Tracks recording elapsed time independently of OBS, so a chapter marker can
// report a timestamp aligned with the recording timeline (paused time excluded).
//
// All times are monotonic milliseconds supplied by the caller ("now"), which
// keeps the class pure and unit-testable. Typical wiring:
//   RECORDING_STARTED  -> start(now)
//   RECORDING_PAUSED   -> pause(now)
//   RECORDING_UNPAUSED -> resume(now)
//   RECORDING_STOPPED  -> stop()
//   add chapter        -> elapsedMs(now)
class RecordingClock {
public:
    void start(int64_t nowMs);
    void pause(int64_t nowMs);
    void resume(int64_t nowMs);
    void stop();

    bool running() const { return running_; }
    bool paused() const { return paused_; }

    // Elapsed recording time in ms, excluding paused spans. 0 when not running.
    int64_t elapsedMs(int64_t nowMs) const;

private:
    bool running_ = false;
    bool paused_ = false;
    int64_t startMs_ = 0;       // when start() was called
    int64_t pausedAccumMs_ = 0; // total paused time before the current pause
    int64_t pauseStartMs_ = 0;  // when the current pause began
};

} // namespace ocm

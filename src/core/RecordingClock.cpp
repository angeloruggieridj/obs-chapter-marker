#include "RecordingClock.hpp"

namespace ocm {

void RecordingClock::start(int64_t nowMs) {
    running_ = true;
    paused_ = false;
    startMs_ = nowMs;
    pausedAccumMs_ = 0;
    pauseStartMs_ = 0;
}

void RecordingClock::pause(int64_t nowMs) {
    if (!running_ || paused_) return;
    paused_ = true;
    pauseStartMs_ = nowMs;
}

void RecordingClock::resume(int64_t nowMs) {
    if (!running_ || !paused_) return;
    paused_ = false;
    pausedAccumMs_ += nowMs - pauseStartMs_;
    pauseStartMs_ = 0;
}

void RecordingClock::stop() {
    running_ = false;
    paused_ = false;
    startMs_ = 0;
    pausedAccumMs_ = 0;
    pauseStartMs_ = 0;
}

int64_t RecordingClock::elapsedMs(int64_t nowMs) const {
    if (!running_) return 0;
    int64_t paused = pausedAccumMs_;
    if (paused_) paused += nowMs - pauseStartMs_;
    int64_t elapsed = nowMs - startMs_ - paused;
    return elapsed < 0 ? 0 : elapsed;
}

} // namespace ocm

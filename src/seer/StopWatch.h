#pragma once

#include <chrono>

namespace seer {

class StopWatch {
    std::chrono::high_resolution_clock::time_point _past;

public:
    StopWatch() {
        reset();
    }

    void reset() {
        _past = std::chrono::high_resolution_clock::now();
    }

    uint64_t msElapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - _past).count();
    }
};

} // namespace seer

#pragma once

#include <chrono>

namespace seer {

class Stopwatch {
    std::chrono::high_resolution_clock::time_point _past;

public:
    Stopwatch() {
        reset();
    }

    void reset() {
        _past = std::chrono::high_resolution_clock::now();
    }

    auto msElapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - _past);
    }
};

} // namespace seer

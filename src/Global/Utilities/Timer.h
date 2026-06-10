#pragma once

#include <cmath>
#include <chrono>

#include <omnetpp.h>

namespace core {

class Timer {
  public:
    Timer() { reset(); }

    [[nodiscard]] double getTime() const {
        const auto now = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_).count());
    }

    [[nodiscard]] double getMicroSeconds() const {
        const auto now = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now - start_).count());
    }

    [[nodiscard]] omnetpp::simtime_t getElapsedSimTime() const {
        const auto procTime = static_cast<int64_t>(std::llround(getMicroSeconds()));
        return omnetpp::SimTime(procTime, omnetpp::SimTimeUnit::SIMTIME_US);
    }

    void reset() { start_ = std::chrono::high_resolution_clock::now(); }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

}  // namespace core

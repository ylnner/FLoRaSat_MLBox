#include "Utils.h"

#include <cmath>
#include <chrono>

namespace core {

namespace {
[[nodiscard]] int uniformInt(omnetpp::cModule *module, int start, int end) {
    return module->intuniform(start, end);
}
}

int randomNumber(omnetpp::cModule *module, int start, int end, int excluded) {
    int value = uniformInt(module, start, end);
    while (value == excluded) {
        value = uniformInt(module, start, end);
    }
    return value;
}

int64_t getMillisSinceEpoch() noexcept {
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

omnetpp::simtime_t getSimtimeSinceStartMillis(int64_t startTimestampMicros) {
    const auto currentMicros = getMillisSinceEpoch();
    return omnetpp::SimTime(currentMicros - startTimestampMicros, omnetpp::SimTimeUnit::SIMTIME_US);
}

double roundTo(double value, int decimals) noexcept {
    const double multiplicator = std::pow(10.0, static_cast<double>(decimals));
    return std::round(value * multiplicator) / multiplicator;
}
}  // namespace core

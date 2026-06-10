#include "ISLState.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace core {

namespace {
[[nodiscard]] std::string toUpperCopy(const std::string &value) {
    std::string upper = value;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upper;
}
}

std::ostream &operator<<(std::ostream &stream, ISLState state) {
    switch (state) {
        case ISLState::WORKING:
            stream << Constants::ISL_STATE_WORKING;
            break;
        case ISLState::DISABLED:
            stream << Constants::ISL_STATE_DISABLED;
            break;
        default:
            throw omnetpp::cRuntimeError("Unsupported ISLState value");
    }
    return stream;
}

std::string to_string(ISLState state) {
    std::ostringstream ss;
    ss << state;
    return ss.str();
}

ISLState fromString(const std::string &text) {
    const auto upper = toUpperCopy(text);
    if (upper == Constants::ISL_STATE_WORKING) {
        return ISLState::WORKING;
    }
    if (upper == Constants::ISL_STATE_DISABLED) {
        return ISLState::DISABLED;
    }
    throw omnetpp::cRuntimeError("Unknown ISLState text constant: %s", text.c_str());
}

}  // namespace core

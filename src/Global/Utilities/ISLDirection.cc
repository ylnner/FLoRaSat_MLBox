#include "ISLDirection.h"

#include <algorithm>
#include <cctype>
#include <sstream>
namespace core {
namespace isldirection {

namespace {
[[nodiscard]] std::string toLowerCopy(const std::string &value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}
}

ISLDirection getCounterDirection(ISLDirection direction) {
    switch (direction) {
        case ISLDirection::LEFT:
            return ISLDirection::RIGHT;
        case ISLDirection::UP:
            return ISLDirection::DOWN;
        case ISLDirection::RIGHT:
            return ISLDirection::LEFT;
        case ISLDirection::DOWN:
            return ISLDirection::UP;
        default:
            throw omnetpp::cRuntimeError("Unsupported ISLDirection value: %d", static_cast<int>(direction));
    }
}

ISLDirection fromString(const std::string &value) {
    const auto lower = toLowerCopy(value);
    if (lower == Constants::ISL_LEFT_NAME) {
        return ISLDirection::LEFT;
    }
    if (lower == Constants::ISL_UP_NAME) {
        return ISLDirection::UP;
    }
    if (lower == Constants::ISL_RIGHT_NAME) {
        return ISLDirection::RIGHT;
    }
    if (lower == Constants::ISL_DOWN_NAME) {
        return ISLDirection::DOWN;
    }
    if (lower == Constants::SAT_GROUNDLINK_NAME) {
        return ISLDirection::GROUNDLINK;
    }
    throw omnetpp::cRuntimeError("Unknown ISLDirection text constant: %s", value.c_str());
}

std::ostream &operator<<(std::ostream &stream, ISLDirection direction) {
    switch (direction) {
        case ISLDirection::LEFT:
            stream << Constants::ISL_LEFT_NAME;
            break;
        case ISLDirection::UP:
            stream << Constants::ISL_UP_NAME;
            break;
        case ISLDirection::RIGHT:
            stream << Constants::ISL_RIGHT_NAME;
            break;
        case ISLDirection::DOWN:
            stream << Constants::ISL_DOWN_NAME;
            break;
        case ISLDirection::GROUNDLINK:
            stream << Constants::SAT_GROUNDLINK_NAME;
            break;
        default:
            throw omnetpp::cRuntimeError("Unsupported ISLDirection value");
    }
    return stream;
}

std::string to_string(ISLDirection direction) {
    std::ostringstream ss;
    ss << direction;
    return ss.str();
}

}  // namespace isldirection
}  // namespace core

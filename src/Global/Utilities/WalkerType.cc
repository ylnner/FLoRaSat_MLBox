#include "WalkerType.h"

#include <algorithm>
#include <cctype>
#include <sstream>
namespace core {
namespace walkertype {

namespace {
[[nodiscard]] std::string toUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}
}

WalkerType parseWalkerType(const std::string &value) {
    const auto upper = toUpperCopy(value);
    if (upper == Constants::WALKERTYPE_DELTA) {
        return WalkerType::DELTA;
    }
    if (upper == Constants::WALKERTYPE_STAR) {
        return WalkerType::STAR;
    }
    throw omnetpp::cRuntimeError("Unknown WalkerType value: %s", value.c_str());
}

std::ostream &operator<<(std::ostream &stream, WalkerType type) {
    switch (type) {
        case WalkerType::DELTA:
            stream << Constants::WALKERTYPE_DELTA;
            break;
        case WalkerType::STAR:
            stream << Constants::WALKERTYPE_STAR;
            break;
        case WalkerType::UNINITIALIZED:
            stream << "UNINITIALIZED";
            break;
        default:
            throw omnetpp::cRuntimeError("Unsupported WalkerType value");
    }
    return stream;
}

std::string to_string(WalkerType type) {
    std::ostringstream ss;
    ss << type;
    return ss.str();
}

}  // namespace walkertype
}  // namespace core
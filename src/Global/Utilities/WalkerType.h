#pragma once

#include <ostream>
#include <string>

#include <omnetpp.h>

#include "Constants.h"

namespace core {
namespace walkertype {

enum class WalkerType {
    UNINITIALIZED,
    DELTA,
    STAR,
};

[[nodiscard]] WalkerType parseWalkerType(const std::string &value);

std::ostream &operator<<(std::ostream &stream, WalkerType type);

[[nodiscard]] std::string to_string(WalkerType type);

}  // namespace walkertype
}  // namespace core

#pragma once

#include <string>

#include <omnetpp.h>

#include "Global/Utilities/Constants.h"

namespace core {

enum class ISLState {
    WORKING,
    DISABLED,
};

std::ostream &operator<<(std::ostream &stream, ISLState state);

[[nodiscard]] std::string to_string(ISLState state);

[[nodiscard]] ISLState fromString(const std::string &text);

}  // namespace core

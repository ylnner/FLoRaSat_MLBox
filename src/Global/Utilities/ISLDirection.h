#pragma once

#include <ostream>
#include <string>

#include <omnetpp.h>

#include "Global/Utilities/Constants.h"
#include "ISLDirection_m.h"
using namespace omnetpp;

namespace core {
namespace isldirection {
ISLDirection getCounterDirection(ISLDirection direction);

ISLDirection fromString(const std::string &value);

std::ostream &operator<<(std::ostream &stream, ISLDirection direction);

std::string to_string(ISLDirection direction);

}  // namespace isldirection
}  // namespace core

#pragma once

#include <sstream>
#include <string>

namespace base {

template <typename Iterator>
[[nodiscard]] std::string toString(Iterator begin, Iterator end) {
    std::stringstream stream;
    stream << "[";
    bool first = true;
    for (; begin != end; ++begin) {
        if (!first) {
            stream << ", ";
        }
        stream << *begin;
        first = false;
    }
    stream << "]";
    return stream.str();
}

}  // namespace core

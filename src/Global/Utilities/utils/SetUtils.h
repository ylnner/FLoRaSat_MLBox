#pragma once

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

namespace base {
namespace set {

template <typename T>
bool contains(const std::set<T> &values, const T &value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

template <typename T>
bool contains(const std::unordered_set<T> &values, const T &value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

template <typename Iterator>
std::string toString(Iterator begin, Iterator end) {
    std::stringstream stream;
    stream << "(";
    bool first = true;
    for (; begin != end; ++begin) {
        if (!first) {
            stream << ", ";
        }
        stream << *begin;
        first = false;
    }
    stream << ")";
    return stream.str();
}

}  // namespace set
}  // namespace core

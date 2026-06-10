#ifndef FLORA_BASE_MACPROTOCOLTYPE_H_
#define FLORA_BASE_MACPROTOCOLTYPE_H_

#include <string>

enum MacProtocolType {
    LORAWAN,
    KMAC,
    UNKNOWN
};

inline MacProtocolType protocolFromString(const std::string& value) {
    if (value == "LORAWAN")
        return LORAWAN;
    if (value == "KMAC")
        return KMAC;
    return LORAWAN;
}

#endif  // FLORA_BASE_MACPROTOCOLTYPE_H_

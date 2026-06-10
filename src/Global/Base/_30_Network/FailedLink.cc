/*
 * FailedLink.cc
 *
 *  Created on: Jan 20, 2026
 *      Author: ylnner
 */




#include "FailedLink.h"
namespace routing {

FailedLink createFailedLink(int srcSat, int dstSat) {
    auto fLink = FailedLink();
    fLink.srcSat = srcSat;
    fLink.dstSat = dstSat;
    return fLink;
}
}  // namespace routing

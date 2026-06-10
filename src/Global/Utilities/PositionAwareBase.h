#pragma once

#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Utilities/libnorad/cSite.h"
//#include "mobility/UniformGroundMobility.h"

namespace core {

class PositionAwareBase {
  public:
    virtual ~PositionAwareBase() = default;
    virtual double getLatitude() const = 0;
    virtual double getLongitude() const = 0;
    virtual double getAltitude() const = 0;  // Expected in kilometers.
    double getDistance(const PositionAwareBase &other);
};

}  // namespace core

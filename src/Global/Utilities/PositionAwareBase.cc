#include "PositionAwareBase.h"

namespace core {

double PositionAwareBase::getDistance(const PositionAwareBase &other) {
    auto pos_1 = cEcef(getLatitude(), getLongitude(), getAltitude());
    auto pos_2 = cEcef(other.getLatitude(), other.getLongitude(), other.getAltitude());
    return pos_1.getDistance(pos_2) / 1000.0;
}

/*
double PositionAwareBase::getDistance(const UniformGroundMobility &other) {
    auto pos_1 = cEcef(getLatitude(), getLongitude(), getAltitude());
    auto pos_2 = cEcef(other.getLatitude(), other.getLongitude(), 0);
    return pos_1.getDistance(pos_2) / 1000.0;
}
*/
}  // namespace core

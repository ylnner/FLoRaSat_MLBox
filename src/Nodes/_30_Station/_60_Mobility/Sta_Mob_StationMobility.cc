/*
 * Sta_Mob_StationMobility.cc
 *
 *  Created on: Oct 24, 2025
 *      Author: root
 */

#include "Sta_Mob_StationMobility.h"
#include "Global/Utilities/libnorad/cEcef.h"


namespace mobility {
Define_Module(Sta_Mob_StationMobility);

void Sta_Mob_StationMobility::initialize(int stage) {
    StationaryMobility::initialize(stage);
    EV << "initializing LUTMotionMobility stage " << stage << endl;
    if (stage == 0) {
        mapx = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapy = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
        latitude = par("latitude");
        longitude = par("longitude");
    }
}

double Sta_Mob_StationMobility::getLongitude() const {
    return longitude;
}

double Sta_Mob_StationMobility::getLatitude() const {
    return latitude;
}

double Sta_Mob_StationMobility::getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude) const {
    cEcef ecefSourceCoord = cEcef(getLatitude(), getLongitude(), 0);  // could change altitude to real value
    cEcef ecefDestCoord = cEcef(refLatitude, refLongitude, refAltitude);     // could change altitude to real value
    return ecefSourceCoord.getDistance(ecefDestCoord);
}

Coord& Sta_Mob_StationMobility::getCurrentPosition() {
    return lastPosition;
}

void Sta_Mob_StationMobility::setInitialPosition() {
    lastPosition.x = ((mapx * longitude) / 360) + (mapx / 2);
    lastPosition.x = static_cast<int>(lastPosition.x) % static_cast<int>(mapx);

    lastPosition.y = ((-mapy * latitude) / 180) + (mapy / 2);
    lastPosition = Coord((((mapx * longitude) / 360) + (mapx / 2)), (((-mapy * latitude) / 180) + (mapy / 2)), 0);
}

}


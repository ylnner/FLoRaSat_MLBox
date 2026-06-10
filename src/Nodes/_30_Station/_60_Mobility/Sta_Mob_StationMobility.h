/*
 * Sta_Mob_StationMobility.h
 *
 *  Created on: Oct 24, 2025
 *      Author: root
 */

#ifndef NODES__30_STATION__60_MOBILITY_STA_MOB_STATIONMOBILITY_H_
#define NODES__30_STATION__60_MOBILITY_STA_MOB_STATIONMOBILITY_H_

#include <omnetpp.h>
#include <cmath>
#include "inet/mobility/static/StationaryMobility.h"  // inet


using namespace inet;

namespace mobility {

//-----------------------------------------------------
// Class: GroundStationMobility
//
// Positions a ground station at a specific lat/long
//-----------------------------------------------------
class Sta_Mob_StationMobility : public StationaryMobility {
   public:

    double getLongitude() const;
    double getLatitude() const;
    virtual Coord& getCurrentPosition() override;

    // returns the Euclidean distance from ground station to reference point - Implemented by Aiden Valentine
    virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

   protected:
    virtual void initialize(int) override;
    virtual void setInitialPosition() override;

    double latitude, longitude;  // Geographic coordinates
    double mapx, mapy;           // Coordinates on map
};

}


#endif /* NODES__30_STATION__60_MOBILITY_STA_MOB_STATIONMOBILITY_H_ */

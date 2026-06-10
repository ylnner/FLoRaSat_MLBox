/*
 * Ter_Mob.h
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */

#ifndef NODES__10_TERMINAL__60_MOBILITY_TER_MOB_H_
#define NODES__10_TERMINAL__60_MOBILITY_TER_MOB_H_

#include <omnetpp.h>

#include "inet/mobility/static/StationaryMobility.h"

using namespace inet;

//-----------------------------------------------------
// Class: Ter_Mob
//
// Positions a LoRa node on ground at a specific lat/long.
// Supports two modes:
// - Uniform distribution around a center point (deploymentRadius > 0)
// - Exact positioning at centerLatitude/centerLongitude (deploymentRadius = 0)
//-----------------------------------------------------
namespace mobility {
class Ter_Mob : public StationaryMobility {
   public:
    Ter_Mob();

    double getLongitude() const;
    double getLatitude() const;
    virtual Coord& getCurrentPosition() override;

    // returns the Euclidean distance from ground station to reference point - Implemented by Aiden Valentine
    virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

   protected:
    virtual void initialize(int) override;
    virtual void setInitialPosition() override;

    double latitude, longitude;              // Geographic coordinates
    double centerLatitude, centerLongitude;  // Central reference point
    double deploymentRadius;                 // Radius in degrees (0 = exact position)
    double mapx, mapy;                       // size of canvas map
};
}


#endif /* NODES__10_TERMINAL__60_MOBILITY_TER_MOB_H_ */

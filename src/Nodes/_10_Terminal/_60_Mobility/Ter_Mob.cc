/*
 * Ter_Mob.cc
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */

#include "Ter_Mob.h"

#include <cmath>

#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Utilities/libnorad/globals.h"

namespace mobility {

Define_Module(Ter_Mob);

Ter_Mob::Ter_Mob() {
    longitude = 0.0;
    latitude = 0.0;
    centerLatitude = 0.0;
    centerLongitude = 0.0;
    deploymentRadius = 0.0;
    mapx = 0;
    mapy = 0;
}

void Ter_Mob::initialize(int stage) {
    StationaryMobility::initialize(stage);
    EV << "initializing Ter_Mob stage " << stage << endl;
    if (stage == 0) {
        mapx = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapy = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
        
        centerLatitude = par("centerLatitude");
        centerLongitude = par("centerLongitude");
        deploymentRadius = par("deploymentRadius");
        
        if (deploymentRadius == 0.0) {
            // Exact position mode: use center coordinates directly
            latitude = centerLatitude;
            longitude = centerLongitude;
            EV << "Ter_Mob: Using exact position - Lat: " << latitude << ", Lon: " << longitude << endl;
        } else {
            // Uniform distribution mode: original behavior
            double r = XKMPER_WGS72;  // earth radius
            double randomRadius = deploymentRadius * sqrt(cComponent::uniform(0, 1));
            double randomAngle = cComponent::uniform(0, TWOPI);

            // reference center point on Earth
            cEcef *P = new cEcef(centerLatitude, centerLongitude, r);
            Coord *Px = new Coord(P->getX(), P->getY(), P->getZ());

            // null island on Earth
            cEcef *O = new cEcef(0, 0, r);
            Coord *Ox = new Coord(O->getX(), O->getY(), O->getZ());

            // get cross and dot products
            Coord cross = *Ox % *Px;
            double angle = Px->angle(*Ox);
            cross.normalize();

            // get rotation quaternion
            Quaternion *q = new Quaternion(cross, angle);

            double xi = (randomRadius / r) * cos(randomAngle) / RADS_PER_DEG;  // longitude
            double yi = (randomRadius / r) * sin(randomAngle) / RADS_PER_DEG;  // latitude

            cEcef *pointEcef = new cEcef(yi, xi, r);
            Coord *pointCoord = new Coord(pointEcef->getX(), pointEcef->getY(), pointEcef->getZ());

            Coord rotatedCoord = q->rotate(*pointCoord);
            cEcef *rotatedEcef = new cEcef(rotatedCoord.getX(), rotatedCoord.getY(), rotatedCoord.getZ(), 0);

            longitude = rotatedEcef->getLongitude();
            latitude = rotatedEcef->getLatitude();

            delete P;
            delete O;
            delete Px;
            delete Ox;
            delete pointEcef;
            delete pointCoord;
            delete rotatedEcef;
            
            EV << "Ter_Mob: Using uniform distribution - Center Lat: " << centerLatitude 
               << ", Center Lon: " << centerLongitude << ", Radius: " << deploymentRadius 
               << " -> Result Lat: " << latitude << ", Lon: " << longitude << endl;
        }
    }
}

double Ter_Mob::getDistance(const double &refLatitude, const double &refLongitude, const double &refAltitude) const {
    // could change altitude to real value
    cEcef ecefSourceCoord = cEcef(latitude, longitude, 0);
    cEcef ecefDestCoord = cEcef(refLatitude, refLongitude, refAltitude);
    return ecefSourceCoord.getDistance(ecefDestCoord);
}

double Ter_Mob::getLongitude() const {
    return longitude;
}

double Ter_Mob::getLatitude() const {
    return latitude;
}

Coord &Ter_Mob::getCurrentPosition() {
    return lastPosition;
}

void Ter_Mob::setInitialPosition() {
    lastPosition.x = ((mapx * longitude) / 360) + (mapx / 2);
    lastPosition.x = static_cast<int>(lastPosition.x) % static_cast<int>(mapx);
    lastPosition.y = ((-mapy * latitude) / 180) + (mapy / 2);
    lastPosition = Coord(lastPosition.x, lastPosition.y, 0);
}
}

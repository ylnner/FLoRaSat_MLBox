/*
 * INorad.h
 *
 *  Created on: Oct 13, 2025
 *      Author: root
 */

#ifndef GLOBAL_MOBILITY_INORAD_H_
#define GLOBAL_MOBILITY_INORAD_H_

#include <omnetpp.h>

#include <ctime>
#include <string>

#include "Global/Utilities/libnorad/cEci.h"
#include "Global/Utilities/libnorad/cJulian.h"
#include "Global/Utilities/libnorad/ccoord.h"

using namespace omnetpp;

namespace Global {

// Interface for adapted OS3 Norad Modules - Written by Aiden Valentine

class INorad : public cSimpleModule {
   public:
    virtual cJulian getJulian() { return currentJulian; };
    // sets the internal calendar by translating the current gregorian time
    // currentTime: time at which the simulation takes place
    virtual void setJulian(std::tm* currentTime);

    virtual void updateTime(const simtime_t& targetTime){};

    // This method gets the current simulation time, cares for the file download (happens only once)
    // of the TLE files from the web and reads the values for the satellites according to the
    // omnet.ini-file. The information is provided by the respective mobility class.
    // targetTime: End time of current linear movement
    virtual void initializeMobility(const simtime_t& targetTime){};

    // returns the longitude
    virtual double getLongitude();

    // returns the latitude
    virtual double getLatitude();

    // returns the elevation to a reference point
    virtual double getElevation(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999);

    // returns the azimuth
    virtual double getAzimuth(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999);

    // returns the altitude
    virtual double getAltitude();

    // returns the distance to the satellite from a reference point (distance in km)
    virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999);

    // returns the number of planes in the constellation
    virtual int getNumberOfPlanes() const { return 1; }

    // returns the number of satellites per plane
    virtual int getSatellitesPerPlane() const { return 1; }

    // returns the Walker constellation spacing parameter (F)
    virtual int getInterPlaneSpacing() const { return 0; }

    // returns the RAAN (Right Ascension of Ascending Node)
    virtual double getRaan() const { return 0.0; }

    // returns the mean anomaly
    virtual double getMnAnomaly() const { return 0.0; }

    // returns the inclination
    virtual double getInclination() const { return 0.0; }

    // returns whether the satellite is ascending
    virtual bool isAscending() const { return true; }

    // returns the future elevation to a reference point with time offset
    virtual double getFutureElevation(simtime_t offset, const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) { return 0.0; }

   protected:
    virtual void handleMessage(cMessage* msg);

   protected:
    cEci eci;
    cJulian currentJulian;

    double gap;
    cCoordGeo geoCoord;
};

}  // namespace global

#endif /* GLOBAL_MOBILITY_INORAD_H_ */

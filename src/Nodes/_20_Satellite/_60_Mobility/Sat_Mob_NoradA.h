#ifndef LEOSATELLITES_MOBILITY_NORADA_H_
#define LEOSATELLITES_MOBILITY_NORADA_H_

#include <omnetpp.h>

#include <cmath>
#include <ctime>
#include <fstream>
#include <string>

#include "Global/Utilities/libnorad/cEci.h"
#include "Global/Utilities/libnorad/cJulian.h"
#include "Global/Utilities/libnorad/cOrbitA.h"
#include "Global/Utilities/libnorad/cSite.h"
#include "Global/Utilities/libnorad/cTLE.h"
#include "Global/Utilities/libnorad/ccoord.h"
#include "Global/Mobility/INorad.h"

using namespace Global;
using namespace omnetpp;

//-----------------------------------------------------
// Class: NoradA
//
// Provides the functionality for satellite positioning
// this class provides the functionality needed to get the positions for satellites according
// to current tables from web information by providing known data
// This class has been adapted from the Norad class so that orbits can be propagated without the
// requirement of a TLE file.
// Modified by Aiden Valentine, original from the OS3 framework
//-----------------------------------------------------

namespace mobility {
class Sat_Mob_NoradA : public INorad {
   public:
    Sat_Mob_NoradA();
    // Updates the end time of current linear movement for calculation of current position
    // targetTime: End time of current linear movement
    virtual void updateTime(const simtime_t& targetTime);

    virtual void finish();
    // This method gets the current simulation time, cares for the file download (happens only once)
    // of the TLE files from the web and reads the values for the satellites according to the
    // omnet.ini-file. The information is provided by the respective mobility class.
    // targetTime: End time of current linear movement
    virtual void initializeMobility(const simtime_t& targetTime);

    double getMnAnomaly();
    double getRaan();
    double getInclination();
    bool isAscending();
    double getFutureElevation(simtime_t offset, double refLatitude, double refLongitude, double refAltitude);

    const int getSatelliteNumber() { return satelliteIndex; };
    const int getNumberOfPlanes() { return planes; }
    const int getSatellitesPerPlane() { return satPerPlane; }
    // Checks if given an index the satellite is a valid inter-satellite link.
    bool isInterSatelliteLink(const int sat2Index);

   private:
    int satelliteIndex;
    int planes;
    int satPerPlane;

    cOrbitA* orbit;
};

}  // namespace mobility
#endif

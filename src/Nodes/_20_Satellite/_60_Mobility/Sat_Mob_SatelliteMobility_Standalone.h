/*
 * Sat_Mob_SatelliteMobility_Standalone.h
 *
 *  Created on: Feb 13, 2026
 *      Author: ylnner
 */

#ifndef NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_SATELLITEMOBILITY_STANDALONE_H_
#define NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_SATELLITEMOBILITY_STANDALONE_H_


#include <cmath>
#include <ctime>
#include <inet/mobility/base/LineSegmentsMobilityBase.h>
#include "inet/common/INETDefs.h"
#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Utilities/libnorad/cJulian.h"
#include "Global/Mobility/INorad.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_NoradA.h"


using namespace Global;
using namespace inet;


//-----------------------------------------------------
// Class: Sat_Mob_SatelliteMobility_Standalone
//
// Realizes the Sat_Mob_SatelliteMobility_Standalone mobility module - provides methods to get and set
// the position of a satellite module and resets the satellite position when
// it gets outside the playground. Code taken from OS3 so that the model can
// work without altering the OS3 code itself.
//-----------------------------------------------------


namespace mobility{

class Sat_Mob_SatelliteMobility_Standalone : public LineSegmentsMobilityBase{
    public:
    Sat_Mob_SatelliteMobility_Standalone();

        // returns x-position of satellite on playground (not longitude!)
        virtual double getPositionX() const { return lastPosition.x; };

        // returns y-position of satellite on playground (not latitude!)
        virtual double getPositionY() const { return lastPosition.y; };

        // returns the altitude of the satellite.
        virtual double getAltitude() const;

        virtual bool isOnSameOrbitalPlane(double raan, double inclination);
        // returns the elevation for the satellite in degrees
        virtual double getElevation(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

        // returns the azimuth from satellite to reference point in degrees
        virtual double getAzimuth(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

        // returns the Euclidean distance from satellite to reference point
        virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

        // returns satellite latitude
        virtual double getLatitude() const;

        // returns satellite longitude
        virtual double getLongitude() const;

        // returns horizontal satellite position (x) in canvas
        double getXCanvas(double lon) const;

        // returns vertical satellite position (y) in canvas
        double getYCanvas(double lat) const;

        ///virtual const inet::Coord& getCurrentVelocity() override;
        Coord getCurrentVelocityEcef() const {
            return currentVelocityEcef;
        }
    protected:
        INorad* noradModule = nullptr;
        int mapX, mapY;
        double transmitPower;
        bool displaySpanArea;
        cPolygonFigure* polygon;
        const int nPoints = 51;
        cCanvas* networkCanvas = nullptr;
        cMessage* refreshArea = nullptr;

        Coord prevPositionEcef;
        Coord currentVelocityEcef;
        bool firstUpdate = true;

        int effectiveSlant = 0;

        virtual int numInitStages() const override { return NUM_INIT_STAGES; }

        // initialize module
        // - creates a reference to the Norad moudule
        // - timestamps and initial position on playground are managed here.
        virtual void initialize(int stage) override;

        virtual void initializePosition() override;

        // sets the position of satellite
        // - sets the target position for the satellite
        // - the position is fetched from the Norad module with reference to the current timestamp
        virtual void setTargetPosition() override;

        // resets the position of the satellite
        // - wraps around the position of the satellite if it reaches the end of the playground
        virtual void fixIfHostGetsOutside();

        // implements basic satellite movement on map
        virtual void move() override;

        // move satellite and update span area
        virtual void handleSelfMessage(cMessage* msg) override;

        // remove all points from span area polygon
        void removeAllPoints();

        // set all points from span area polygon
        void setAllPoints();
};

} // namespace mobility




#endif /* NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_SATELLITEMOBILITY_STANDALONE_H_ */

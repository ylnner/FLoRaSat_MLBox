/*
 * ConstellationCreator.h
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */

#ifndef GLOBAL_CONSTELLATION_CREATOR_CONSTELLATIONCREATOR_H_
#define GLOBAL_CONSTELLATION_CREATOR_CONSTELLATIONCREATOR_H_

#include <omnetpp.h>
#include <string.h>

#include "Global/Mobility/INorad.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_NoradA.h"
#include "Global/Utilities/Utils.h"

using namespace omnetpp;

namespace Global{
namespace constellation{

class ConstellationCreator : public cSimpleModule{
    public:
        ConstellationCreator();

    protected:
        void createSatellites();
        void createSatellite(int index, double raan, double meanAnomaly, int plane);
        virtual void initialize() override;
        // virtual void handleMessage(cMessage *msg) override;


    protected:
         /** @brief Number of satellites in the constellation. */
         int satCount;

         /** @brief Number of planes in the constellation. */
         int planeCount;

         /** @brief Number of satellites in every plane. */
         int satsPerPlane;

         /** @brief Constellation inclination, given in deg. */
         double inclination;

         /** @brief Orbit height, given in km */
         int altitude;

         /** @brief The relative spacing between satellites in adjacent planes. */
         int interPlaneSpacing;

         /** @brief The raan spread of the constellation. 180 for Walker Star, 360 for Walker Delta. */
         int raanSpread;

         /** @brief Eccentricity of the constellation. */
         double eccentricity;

         int baseYear;
         double baseDay;
         int epochYear;
         double epochDay;
};

}//Constellation
}//Global
#endif /* GLOBAL_CONSTELLATION_CREATOR_CONSTELLATIONCREATOR_H_ */

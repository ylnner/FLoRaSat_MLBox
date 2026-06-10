/*
 * Sat_Mob_Orchestator.h
 *
 *  Created on: Oct 28, 2025
 *      Author: root
 */

#ifndef NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_ORCHESTRATOR_H_
#define NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_ORCHESTRATOR_H_

#include <omnetpp.h>

#include "Global/Utilities/Timer.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "Global/Mobility/INorad.h"
#include "Global/Mobility/INorad.h"
#include "Sat_Mob_SatelliteMobility.h"


namespace mobility {

using SatMobVector = std::vector<Sat_Mob_SatelliteMobility *>;

class Sat_Mob_Orchestrator : public inet::ClockUserModuleMixin<omnetpp::cSimpleModule> {
   public:
    Sat_Mob_Orchestrator();

   protected:
    virtual ~Sat_Mob_Orchestrator();
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    void updatePositions();
    /** @brief Schedules the update timer that will update the topology state.*/
    void scheduleUpdate();
    /** @brief Loads the SatelliteMobility of all satellites. */
    SatMobVector loadSatMobilities();

   protected:
    /** @brief The simulation time interval used to regularly signal mobility state changes. */
    cPar *updateIntervalParameter = nullptr;
    inet::ClockEvent *updateTimer = nullptr;

    /** @brief Used to store sat mobilities. */
    SatMobVector satMobVector;
    int numSatellites;

};

}  // namespace mobility


#endif /* NODES__20_SATELLITE__60_MOBILITY_SAT_MOB_ORCHESTRATOR_H_ */

/*
 * Sat_Mob_Orchestator.cc
 *
 *  Created on: Oct 28, 2025
 *      Author: root
 */


#include "Sat_Mob_Orchestrator.h"
#include <algorithm>

using namespace mobility;
using namespace core;
namespace mobility {

Define_Module(mobility::Sat_Mob_Orchestrator);

Sat_Mob_Orchestrator::Sat_Mob_Orchestrator() : updateTimer(nullptr),
                                                     updateIntervalParameter(0) {
}

Sat_Mob_Orchestrator::~Sat_Mob_Orchestrator() {
    cancelAndDeleteClockEvent(updateTimer);
}

void Sat_Mob_Orchestrator::initialize(int stage) {
    // NOTE: Avoid using inet::INITSTAGE_* constants here. On some OMNeT++/INET
    // builds the init-stage registry conversion may crash during dynamic linking.
    // Stage 0 is always the local initialization stage.
    if (stage == 0) { // INITSTAGE_LOCAL
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new inet::ClockEvent("UpdateTimer");
    }
    // Load satellite mobility modules early, but do NOT call updatePosition() during
    // initialization. Some mobilities are not fully initialized yet at this point.
    else if (stage == 1) { // early init stage after local
        satMobVector = loadSatMobilities();
    }
    // Schedule the first update after initialization.
    else if (stage == 2) {
        if (updateTimer != nullptr && !updateTimer->isScheduled()) {
            // Trigger an immediate update at t=0 as a normal event (after init).
            scheduleClockEventAfter(0.0, updateTimer);
        }
    }
}

SatMobVector Sat_Mob_Orchestrator::loadSatMobilities() {
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in SatMobilityOrchestrator::loadSatellites: Could not find parent.");
    }
    numSatellites = getSystemModule()->getSubmoduleVectorSize("satellite");
    SatMobVector satellites;
    satellites.reserve(numSatellites);
    for (size_t i = 0; i < numSatellites; i++) {
        cModule *sat = getSystemModule()->getSubmodule("satellite", i);
        if (sat == nullptr) {
            error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat with id %d.", (int)i);
        }
        Sat_Mob_SatelliteMobility *satMobility = check_and_cast<Sat_Mob_SatelliteMobility *>(sat->getSubmodule("mobility"));
        if (satMobility == nullptr) {
            error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat mobility for sat %d.", (int)i);
        }
        satellites.emplace_back(satMobility);

        // INorad *noradModule = check_and_cast<INorad *>(sat->getSubmodule("NoradModule"));
        // if (satMobility == nullptr) {
        //     error("Error in SatMobilityOrchestrator::loadSatellites: Could not find sat norad module for sat %d.", i);
        // }
        // noradModule->initializeMobility(simTime());
    }
    return satellites;
}

// int SatMobilityOrchestrator::calculateSliceCount(int satMobilityCount) {
//     return 12;
// }

// void SatMobilityOrchestrator::createSatSlices(int sliceCount, Slice slice) {
//     // fill the map with empty vectors to store the slices
//     for (size_t i = 0; i < sliceCount; i++) {
//         Slice t;
//         satMobilitySlices.emplace(i, t);
//     }
//     // distribute ptrs to the sat mobilities between all slices
//     size_t index = 0;
//     while (!slice.empty()) {
//         inet::SatelliteMobility* satMobility = slice.back();
//         slice.pop_back();
//         satMobilitySlices.at(index).push_back(satMobility);
//         index = ++index % sliceCount;
//     }
// }

void Sat_Mob_Orchestrator::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updatePositions();
        // Re-schedule periodically if the interval is positive.
        if (updateIntervalParameter != nullptr && updateIntervalParameter->doubleValue() > 0) {
            scheduleUpdate();
        }
    } else
        error("SatMobilityOrchestrator: Unknown message.");
}

void Sat_Mob_Orchestrator::updatePositions() {
    // This can be called at t=0 right after initialization. Be defensive.
    if (numSatellites == 0 || satMobVector.empty()) {
        EV_WARN << "Sat_Mob_Orchestrator: no satellites/mobilities loaded; skipping update." << endl;
        return;
    }

    const size_t n = std::min(static_cast<size_t>(numSatellites), satMobVector.size());
    EV << "Update satellite positions (n=" << n << ")." << endl;

    Timer timer = core::Timer();
    SimTime currentTime = simTime();
    for (size_t i = 0; i < n; i++) {
        auto *mob = satMobVector[i];
        if (mob == nullptr) {
            EV_WARN << "Sat_Mob_Orchestrator: null mobility for satellite[" << i << "]; skipping." << endl;
            continue;
        }
        mob->updatePosition(currentTime);
    }

    EV << "FSM: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
}

void Sat_Mob_Orchestrator::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

}  // namespace mobility

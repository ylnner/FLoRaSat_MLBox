/*
 * ConstellationTopologyControl.cc
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */


#include "ConstellationTopologyControl.h"

#include <algorithm>

using namespace inet;
using namespace omnetpp;
using namespace routing;

namespace topologycontrol {

Define_Module(ConstellationTopologyControl);

void ConstellationTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        timeSliceInterval = par("timeSliceLength").doubleValue();
        timeSliceTimer = new ClockEvent("TimeSliceTimer");
        if (timeSliceInterval > 0) {
            scheduleClockEventAfter(timeSliceInterval, timeSliceTimer);
        }
    }
    
        else if (stage == INITSTAGE_PHYSICAL_LAYER) {
        EV_INFO << "Reserve for " << numSatellites << " sat routing bases" << endl;
        satRoutingBases.reserve(numSatellites);
        for (size_t i = 0; i < numSatellites; i++) {
            satRoutingBases.emplace_back(check_and_cast<routing::Base_Routing *>(getParentModule()->getSubmodule("satellite", i)->getSubmodule("net")->getSubmodule("routing")));
        }

        // If terminals exists
        try {
            if(getSystemModule()->findSubmodule("terminal", 0) != -1){
                EV << "Reserve for " << numLoraDevices << " lora devices " << endl;
                devicesRoutingBases.reserve(numLoraDevices);
                for (size_t i = 0; i < numLoraDevices; i++) {
                    devicesRoutingBases.emplace_back(check_and_cast<routing::Ter_Base_Routing *>(getParentModule()->getSubmodule("terminal", i)->getSubmodule("net")->getSubmodule("routing")));
                }
            }
        } catch (cRuntimeError& e) {
            EV << "Error terminals" << endl;
        }


        // if gs receiver exists
        /*try {
            if(getSystemModule()->findSubmodule("station", 0) != -1){
                EV << "Reserve for " << numGroundStations << " gs routing bases" << endl;
                gsRoutingBases.reserve(numGroundStations);
                for (size_t i = 0; i < numGroundStations; i++) {
                    gsRoutingBases.emplace_back(check_and_cast<routing::Ter_Base_Routing *>(getParentModule()->getSubmodule("station", i)->getSubmodule("net")->getSubmodule("routing")));
                }
            }

        }catch (cRuntimeError& e) {
            EV << "Error Station" << endl;
        }
        */

    }
}

void ConstellationTopologyControl::initTopology() {
    Enter_Method("initTopology");
    EV_INFO << "ConstellationTopologyControl::initTopology()" <<endl;

    topologyChanged = false;
    // create intra-plane and inter-plane ISL links and ground links

    updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinks();
    updateLoraDevices();

    if (topologyChanged) {
        EV_INFO << "Entre topologyChanged: " << topologyChanged <<endl;
        trackTopologyChange();
        lastChange = simTime();
    }
}

void ConstellationTopologyControl::handleMessage(cMessage *msg) {
    Enter_Method("handleMessage");
    if (msg == timeSliceTimer) {
        EV_INFO << "Handle timeSliceTimer at " << simTime() << endl;
        EV_DEBUG << "Handle timeSliceTimer at " << simTime() << endl;
        topologyChangeByTimeSliceTimer = true;
        trackTopologyChange();
        topologyChangeByTimeSliceTimer = false;
        cancelClockEvent(timeSliceTimer);
        scheduleClockEventAfter(timeSliceInterval, timeSliceTimer);
    } else {
        EV_INFO << "TopologyControlBase::handleMessage(msg)" <<endl;
        TopologyControlBase::handleMessage(msg);
    }
};


bool ConstellationTopologyControl::isStillConnectedAt(int gsId, int satId, simtime_t offset, bool isGS) {
    Enter_Method("isStillConnectedAt");
    // check if update occurs until then
    simtime_t now = simTime();
    simtime_t time = now + offset;

    simtime_t timeSliceTime = timeSliceTimer->getArrivalTime();
    simtime_t updateTime = updateTimer->getArrivalTime();

    if ((timeSliceTime > time) && updateTime > time) {
        return true;
    }

    // auto gs = getGroundstationInfo(gsId);
    //auto ref = isGS ? getGroundstationInfo(gsId) : getDeviceInfo(gsId);

    auto sat = getSatellite(satId);
    if (isGS){
        auto gs = getGroundstationInfo(gsId);
        double fElevation = sat->getFutureElevation(*gs, offset);
        //double fElevation = sat->getFutureElevation((check_and_cast<Sta *>(gs))->getSubmodule("mobility"), offset);
        /*double fElevation = sat->getFutureElevation(
            *check_and_cast<core::PositionAwareBase*>(
                (check_and_cast<Sta *>(gs))->getSubmodule("mobility")
            ),
            offset
        );*/

        return fElevation >= minimumElevation;
    }else{
        auto dev = getDeviceInfo(gsId);
        double fElevation = sat->getFutureElevation(*dev, offset);
        /*double fElevation = sat->getFutureElevation(
                    *check_and_cast<core::PositionAwareBase*>(
                        (check_and_cast<Ter *>(dev))->getSubmodule("mob")
                    ),
                    offset
        );*/
        return fElevation >= minimumElevation;
    }

    // EV_DEBUG << "Current: " << sat->getElevation(*gs) << "; Future: " << fElevation << ";" << endl;

}


void ConstellationTopologyControl::updateIntraSatelliteLinks() {
    Enter_Method("updateIntraSatelliteLinks");
    // if intra-plane ISL is not enabled/available
    if (intraPlaneIslDisabled) return;
    
    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        int planeStart = static_cast<int>(plane) * satsPerPlane;
        if (planeStart >= numSatellites) {
            break;
        }
        int planeEndExclusive = allowUnevenPlanes
                                     ? std::min(planeStart + satsPerPlane, numSatellites)
                                     : (planeStart + satsPerPlane);

        for (int index = planeStart; index < planeEndExclusive; index++) {
            Sat *curSat = satellites.at(index);
            ASSERT(curSat != nullptr);

            int nextId = (index + 1 >= planeEndExclusive) ? planeStart : index + 1;
            Sat *otherSat = satellites.at(nextId);
            ASSERT(otherSat != nullptr);

            // connect the satellites
            connectSatellites(curSat, otherSat, isldirection::ISLDirection::UP);
        }
    }
}

void ConstellationTopologyControl::updateInterSatelliteLinks() {
    Enter_Method("updateInterSatelliteLinks");
    // if inter-plane ISL is not enabled/available
    if (interPlaneIslDisabled) return;

    switch (walkerType) {
        case WalkerType::DELTA:
            updateISLInWalkerDelta();
            break;
        case WalkerType::STAR:
            updateISLInWalkerStar();
            break;
        default:
            error("Error in ConstellationTopologyControl::updateInterSatelliteLinks(): Unexpected WalkerType '%s'.", to_string(walkerType).c_str());
    }
}

void ConstellationTopologyControl::updateISLInWalkerDelta() {
    for (size_t index = 0; index < numSatellites; index++) {
        Sat *curSat = satellites.at(index);
        ASSERT(curSat != nullptr);

        // sat (o, i) = i-th sat in plane o
        // F = interplanefacing angle
        int satPlane = curSat->getPlane();
        Sat *rightSat = (satPlane != planeCount - 1)
                                             ? findSatByPlaneAndNumberInPlane(satPlane + 1, curSat->getNumberInPlane())
                                             : findSatByPlaneAndNumberInPlane(0, (curSat->getNumberInPlane() + interPlaneSpacing) % satsPerPlane);
        // SatelliteRoutingBase *rightSat = findSatByPlaneAndNumberInPlane((satPlane + 1) % planeCount, curSat->getNumberInPlane());
        if (rightSat == nullptr) {
            continue;
        }
        connectSatellites(curSat, rightSat, isldirection::ISLDirection::RIGHT);
        // if (curSat->isAscending()) {
        //     // if next plane partner is descending, connection is not possible
        //     if (rightSat->isDescending()) {
        //         // // if we were connected to that satellite on right
        //         // if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
        //         //     disconnectSatellites(curSat, rightSat, isldirection::ISLDirection::RIGHT);
        //         // }
        //         // // if we were connected to that satellite on left
        //         // else if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
        //         //     disconnectSatellites(curSat, rightSat, isldirection::ISLDirection::LEFT);
        //         // }
        //     } else {
        //     }
        //     connectSatellites(curSat, rightSat, isldirection::ISLDirection::RIGHT);
        // }
        // // sat ist descending
        // else {
        //     // if next plane partner is not descending, connection is not possible
        //     if (rightSat->isAscending()) {
        //         // // if we were connected to that satellite on right
        //         // if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
        //         //     disconnectSatellites(curSat, rightSat, isldirection::ISLDirection::LEFT);
        //         // }
        //         // // if we were connected to that satellite on right
        //         // else if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
        //         //     disconnectSatellites(curSat, rightSat, isldirection::ISLDirection::RIGHT);
        //         // }
        //     } else {
        //     }
        //     connectSatellites(curSat, rightSat, isldirection::ISLDirection::LEFT);
        // }
    }
}

void ConstellationTopologyControl::updateISLInWalkerStar() {

    EV_INFO << "ConstellationTopologyControl::updateISLInWalkerStar "<<endl;
    EV << "numSatellites: " << numSatellites <<endl;
    EV << "planeCount: "<< planeCount <<endl;
    for (size_t index = 0; index < numSatellites; index++) {
        EV_INFO << "---------------------"<<endl;
        Sat *curSat = satellites.at(index);
        EV << "curSat: " << curSat->getId() << " - "<< index <<endl;
        EV_INFO << "1 -" <<endl;
        ASSERT(curSat != nullptr);
        int satPlane = curSat->getPlane();
        EV << "satPlane: "<< satPlane <<endl;

        bool isLastPlane = satPlane == planeCount - 1;

        // if last plane stop because we reached the seam
        if (isLastPlane)
            break;
        int rightIndex = (index + satsPerPlane) % numSatellites;
        Sat *nextPlaneSat = satellites.at(rightIndex);
        EV << "nextPlaneSat: " << nextPlaneSat->getId() << " - "<< rightIndex <<endl;
        EV_INFO << "2 -" <<endl;
        ASSERT(nextPlaneSat != nullptr);

        // if inter-plane isl is disabled for any satellite, just check to disconnect them
        bool curSatIsAscending = curSat->isAscending();
        bool nextPlaneSatIsAscending = nextPlaneSat->isAscending();
        if (!curSat->isInterPlaneISLEnabled() || !nextPlaneSat->isInterPlaneISLEnabled() || curSatIsAscending != nextPlaneSatIsAscending) {
            EV_INFO << "3 -" <<endl;
            // if we were connected to that satellite on right
            if (curSat->hasRightSat() && curSat->getRightSatId() == nextPlaneSat->getId()) {
                disconnectSatellites(curSat, nextPlaneSat, isldirection::ISLDirection::RIGHT);
            }
            // if we were connected to that satellite on left
            else if (curSat->hasLeftSat() && curSat->getLeftSatId() == nextPlaneSat->getId()) {
                disconnectSatellites(curSat, nextPlaneSat, isldirection::ISLDirection::LEFT);
            }
        }
        // sats are moving up
        else if (curSatIsAscending) {
            EV_INFO << "4 -" <<endl;
            connectSatellites(curSat, nextPlaneSat, isldirection::ISLDirection::RIGHT);
        }
        // sats are moving down
        else {
            EV_INFO << "5 -" <<endl;
            connectSatellites(curSat, nextPlaneSat, isldirection::ISLDirection::LEFT);
        }
    }
    EV_INFO << "End ConstellationTopologyControl::updateISLInWalkerStar" <<endl;
}

void ConstellationTopologyControl::updateGroundstationLinks() {
    Enter_Method("updateGroundstationLinks");
    // iterate over groundstations
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        Sta *gs = groundStations.at(gsId);
        ASSERT(gs != nullptr);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            Sat *sat = satellites.at(satId);

            ASSERT(sat != nullptr);

            bool isOldConnection = gs->isConnectedTo(satId);
            double elevation = sat->getElevation(*gs);

            // Debug: print elevation for GS 5 at first update
            if (gsId == 5 && simTime() < 10) {
                EV_DETAIL << "t=" << simTime() << " GS5-Sat" << satId << " elev=" << elevation << endl;
            }

            // if is not in range continue with next sat
            if (elevation < minimumElevation) {
                // if they were previous connected, delete that connection
                if (isOldConnection) {
                    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                    cGate *uplink = gs->getOutputGate(connection.gsGateIndex);
                    cGate *downlink = sat->getOutputGate(isldirection::ISLDirection::GROUNDLINK, connection.satGateIndex).first;
                    deleteChannel(uplink);
                    deleteChannel(downlink);
                    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
                    gs->removeSatellite(satId);
                    topologyChanged = true;
                }
                // in all cases continue with next sat
                continue;
            }
            double delay = sat->getDistance(*gs) * groundlinkDelay;

            // if they were previous connected, update channel with new delay
            if (isOldConnection) {
                GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                cGate *uplinkO = gs->getOutputGate(connection.gsGateIndex);
                cGate *uplinkI = gs->getInputGate(connection.gsGateIndex);
                cGate *downlinkO = sat->getOutputGate(isldirection::ISLDirection::GROUNDLINK, connection.satGateIndex).first;
                cGate *downlinkI = sat->getInputGate(isldirection::ISLDirection::GROUNDLINK, connection.satGateIndex).first;
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
            }
            // they were not previous connected, create new channel between gs and sat
            else {
                int freeIndexGs = -1;
                int connectedCount = 0;
                for (size_t i = 0; i < numGroundLinks; i++) {
                    cGate *gate = gs->getOutputGate(i);
                    if (!gate->isConnectedOutside()) {
                        freeIndexGs = i;
                        break;
                    } else {
                        connectedCount++;
                    }
                }
                if (freeIndexGs == -1) {
                    std::stringstream ss;
                    ss << "No free gs gate index found. GS " << gsId
                       << ": numGroundLinks=" << numGroundLinks
                       << ", connectedGates=" << connectedCount
                       << ", trackedSats=" << gs->getSatellites().size();
                    error("%s", ss.str().c_str());
                }

                int freeIndexSat = -1;
                for (size_t i = 0; i < numGroundLinks; i++) {
                    const cGate *gate = sat->getOutputGate(isldirection::ISLDirection::GROUNDLINK, i).first;
                    if (!gate->isConnectedOutside()) {
                        freeIndexSat = i;
                        break;
                    }
                }
                if (freeIndexSat == -1) {
                    error("No free sat gate index found.");
                }

                cGate *uplinkO = gs->getOutputGate(freeIndexGs);
                cGate *uplinkI = gs->getInputGate(freeIndexGs);
                cGate *downlinkO = sat->getOutputGate(isldirection::ISLDirection::GROUNDLINK, freeIndexSat).first;
                cGate *downlinkI = sat->getInputGate(isldirection::ISLDirection::GROUNDLINK, freeIndexSat).first;
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);

                gsSatConnections.emplace(std::pair<int, int>(gsId, satId), GsSatConnection(gsId, satId, freeIndexGs, freeIndexSat));
                gs->addSatellite(satId);
                topologyChanged = true;
            }
        }
    }
}

void ConstellationTopologyControl::updateLoraDevices(){
    Enter_Method("updateLoraDevices");

    // iterate over lora nodes
    for (size_t loraId = 0; loraId < numLoraDevices; loraId++){
        Ter *ld = loraDevices.at(loraId);

        ASSERT(ld != nullptr);
        for (size_t satId = 0; satId < numSatellites; satId++){
            Sat *sat = satellites.at(satId);
            ASSERT(sat != nullptr);

            bool isOldConnection = ld->isConnectedTo(satId);
            //UniformGroundMobility * mobility = lp->getMobility();
            if ( sat->getElevation(*ld) < minimumElevation){
                if (isOldConnection){
                    GsSatConnection &connection = deviceSatConnections.at(std::pair<int, int>(loraId, satId));
                    deviceSatConnections.erase(std::pair<int, int>(loraId, satId));

                    ld->removeSatellite(satId);
                    topologyChanged = true;
                }
                continue;
            }

            double delay = sat->getDistance(*ld) * groundlinkDelay;
            // if they were previous connected, update channel with new delay
            if (isOldConnection) {
                /*GsSatConnection &connection = deviceSatConnections.at(std::pair<int, int>(loraId, satId));
                cGate *uplinkO              = ld->getOutputGate(connection.gsGateIndex);
                cGate *uplinkI              = ld->getInputGate(connection.gsGateIndex);
                cGate *downlinkO            = sat->getOutputGate(isldirection::ISLDirection::GROUNDLINK, connection.satGateIndex).first;
                cGate *downlinkI            = sat->getInputGate(isldirection::ISLDirection::GROUNDLINK, connection.satGateIndex).first;
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);*/
            }else{
                int freeIndexLora = 1;
                int freeIndexSat  = 1;
                deviceSatConnections.emplace(std::pair<int, int>(loraId, satId), GsSatConnection(loraId, satId, freeIndexLora, freeIndexSat));
                EV << "Previous to  load Satellite satId: " << satId<< endl;
                ld->addSatellite(satId);
                topologyChanged = true;

            }
        }
    }

}


void ConstellationTopologyControl::updateTopology() {
    Enter_Method("updateTopology");
    if (simTime() <= lastChange) {
        EV_DEBUG << "Topology was already changed at this time." << endl;
        return;
    }
    EV_DEBUG << "Check Topo for update." << endl;
    //core::Timer timer = core::Timer();
    topologyChanged = false;
    // update inter-plane ISL links and ground links
    updateInterSatelliteLinks();
    ////// ACHF
    updateLoraDevices();
    updateGroundstationLinks();

    //EV_INFO << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged) {
        EV_INFO << "topologyChanged is true" <<endl;
        trackTopologyChange();
        lastChange = simTime();
        if (timeSliceInterval > 0) {
            cancelClockEvent(timeSliceTimer);
            scheduleClockEventAfter(timeSliceInterval, timeSliceTimer);
        }
    }
}

void ConstellationTopologyControl::trackTopologyChange() {
    Enter_Method("trackTopologyChange");
    EV_INFO << "Topology change at " << simTime() << "; Topology changed: " << !topologyChangeByTimeSliceTimer << endl;


    // update connection matrix
    if (!topologyChangeByTimeSliceTimer) {
        EV_INFO << "Calculates costMatrix"<<endl;
        costMatrix = routing::core::dspa::buildShortestPathCostMatrix(numSatellites, satellites);
    }


    // notify routing module about topology change.
    // Not all algorithms are required to react to this, if they are purely dynamic.
    // This call can be used to simulate pre-planned routing tables.
    Timer timer = Timer();
    for (auto satBase : satRoutingBases) {
        satBase->handleTopologyChange(!topologyChangeByTimeSliceTimer);
    }
    EV_INFO << "Sat TopoChange took " << timer.getTime() / 1000 / 1000 << "ms" << endl;


    Timer timer3 = Timer();
    for (auto deviceBase : devicesRoutingBases) {
        EV << "auto deviceBase : devicesRoutingBases" <<endl;
        deviceBase->handleTopologyChangeDevices(!topologyChangeByTimeSliceTimer);
    }
    EV << "Devices TopoChange took " << timer3.getTime() / 1000 / 1000 << "ms" << endl;

    Timer timer2 = Timer();
    for (auto gsBase : gsRoutingBases) {
        gsBase->handleTopologyChange(!topologyChangeByTimeSliceTimer);
    }
    EV << "Gs TopoChange took " << timer2.getTime() / 1000 / 1000 << "ms" << endl;
    
}

ConstellationTopologyControl::~ConstellationTopologyControl() {
    //cancelAndDeleteClockEvent(timeSliceTimer);
}

}

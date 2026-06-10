/*
 * TopologyControlBase.cc
 *
 *  Created on: Oct 14, 2025
 *      Author: root
 */


#include "TopologyControlBase.h"

#include <algorithm>
//#include "LoRaApp/SimpleLoRaApp.h"
#include "Nodes/_10_Terminal/Ter.h"
//#include "LoRa/LoRaGWRadio.h"

#include "Global/Mobility/INorad.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"

using namespace core;

namespace topologycontrol {

Register_Abstract_Class(TopologyControlBase);
//Define_Module(TopologyControlBase);

TopologyControlBase::TopologyControlBase() : topologyChanged(false),
                                             satsPerPlane(0),
                                             numGroundStations(0),
                                             numSatellites(0),
                                             planeCount(0),
                                             interPlaneSpacing(1),
                                             minimumElevation(5.0),
                                             numGroundLinks(60),
                                             groundlinkDatarate(0.0),
                                             groundlinkDelay(0.0),
                                             islDatarate(0.0),
                                             islDelay(0.0),
                                             intraPlaneIslDisabled(false),
                                             interPlaneIslDisabled(false),
                                             allowUnevenPlanes(false),
                                             walkerType(WalkerType::UNINITIALIZED),
                                             updateTimer(nullptr),
                                             updateIntervalParameter(0) {
}

TopologyControlBase::~TopologyControlBase() {
    cancelAndDeleteClockEvent(updateTimer);
}

void TopologyControlBase::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        // loaded properties
        updateIntervalParameter = &par("updateInterval");
        updateTimer             = new ClockEvent("UpdateTimer");
        walkerType              = parseWalkerType(par("walkerType"));
        intraPlaneIslDisabled   = par("intraPlaneIslDisabled");
        interPlaneIslDisabled   = par("interPlaneIslDisabled");
        islDelay                = par("islDelay");
        islDatarate             = par("islDatarate");
        groundlinkDelay         = par("groundlinkDelay");
        groundlinkDatarate      = par("groundlinkDatarate");
        numGroundLinks          = par("numGroundLinks");
        minimumElevation  = par("minimumElevation");
        interPlaneSpacing       = par("interPlaneSpacing");
        planeCount              = par("planeCount");
        allowUnevenPlanes       = par("allowUnevenPlanes");

        EV << "TC: Loaded parameters: "
           << "updateInterval: " << updateIntervalParameter << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "minimumElevation: " << minimumElevation << endl;

    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        if(getSystemModule()->findSubmodule("satellite", 0) != -1){
            numSatellites = getSystemModule()->getSubmoduleVectorSize("satellite");
        }else{
            EV << "Topology Base, satellites not found"<<endl;
        }
        //check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));

        if(getSystemModule()->findSubmodule("station", 0) != -1){
            numGroundStations = getSystemModule()->getSubmoduleVectorSize("station");
        }else{
            EV << "Topology Base, ground station not found"<<endl;
        }

        if(getSystemModule()->findSubmodule("terminal", 0) != -1){
            numLoraDevices = getSystemModule()->getSubmoduleVectorSize("terminal");
        }else{
            EV << "Topology Base, lora device not found"<<endl;
        }

        if (planeCount <= 0 || interPlaneSpacing < 0) {
            resolveConstellationParametersFromNorad();
        }

        EV << "numSatellites: " << numSatellites << "; "
                << "numGroundStations: " << numGroundStations << "; "
                << "numLoraDevices: " << numLoraDevices << "; "
                << "numGroundStationsTransmitter: " << numGroundStationsTransmitter << "; "<< endl;

    } else if (stage == inet::INITSTAGE_LINK_LAYER) {
        if (planeCount <= 0 || interPlaneSpacing < 0) {
            resolveConstellationParametersFromNorad();
        }
        // calculated properties
        if (allowUnevenPlanes) {
            auto *noradModule = getNoradModule();
            if (noradModule != nullptr) {
                int derivedPlaneCount = noradModule->getNumberOfPlanes();
                if (derivedPlaneCount > 0 && (planeCount <= 0 || numSatellites % planeCount != 0)) {
                    planeCount = derivedPlaneCount;
                    par("planeCount").setIntValue(planeCount);
                    EV_WARN << "TopologyControlBase: Overriding planeCount to " << planeCount
                            << " to keep consistency with NoradModule in uneven-plane mode." << endl;
                }

                int derivedSatsPerPlane = noradModule->getSatellitesPerPlane();
                if (derivedSatsPerPlane > 0) {
                    satsPerPlane = derivedSatsPerPlane;
                }
            }

            if (planeCount <= 0) {
                planeCount = 1;
                par("planeCount").setIntValue(planeCount);
                EV_WARN << "TopologyControlBase: planeCount was not set; defaulting to 1 in uneven-plane mode." << endl;
            }

            if (satsPerPlane <= 0) {
                satsPerPlane = numSatellites / std::max(planeCount, 1);
            }

            if (interPlaneSpacing < 0 || interPlaneSpacing > planeCount - 1) {
                int clampedSpacing = std::clamp(interPlaneSpacing, 0, std::max(planeCount - 1, 0));
                EV_WARN << "TopologyControlBase: interPlaneSpacing=" << interPlaneSpacing
                        << " out of range for planeCount=" << planeCount
                        << ", clamping to " << clampedSpacing << " in uneven-plane mode." << endl;
                interPlaneSpacing = clampedSpacing;
                par("interPlaneSpacing").setIntValue(interPlaneSpacing);
            }
        } else {
            satsPerPlane = numSatellites / planeCount;
        }
        raanDelta = (walkerType == WalkerType::DELTA ? 360.0 : 180.0) / planeCount;  // ΔΩ = 2𝜋/𝑃 in [0,2𝜋]
        phaseDiff = 360.0 / satsPerPlane;                                            // ΔΦ = 2𝜋/Q in [0,2𝜋]
        phaseOffset = (360.0 * interPlaneSpacing) / numSatellites;                   // Δ𝑓 = 2𝜋𝐹/𝑃𝑄 in [0,2𝜋[
        // std::cout << "ΔΩ=" << raanDelta << "; ΔΦ=" << phaseDiff << "; Δf=" << phaseOffset << endl;

        // validate state

        VALIDATE(raanDelta >= 0.0 && raanDelta <= 360.0);
        VALIDATE(phaseDiff >= 0.0 && phaseDiff <= 360.0);
        VALIDATE(phaseOffset >= 0.0 && phaseOffset < 360.0);
        VALIDATE(walkerType != WalkerType::UNINITIALIZED);
        VALIDATE(numGroundLinks > 0);
        if (!allowUnevenPlanes) {
        VALIDATE(interPlaneSpacing <= planeCount - 1 && interPlaneSpacing >= 0);
        } else if (interPlaneSpacing < 0 || interPlaneSpacing > planeCount - 1) {
            EV_WARN << "TopologyControlBase: interPlaneSpacing still out of range after clamping ("
                    << interPlaneSpacing << ")." << endl;
        }
        VALIDATE(numSatellites > 0);
        VALIDATE(planeCount > 0);
        VALIDATE(satsPerPlane > 0);
        if (!allowUnevenPlanes) {
        VALIDATE(satsPerPlane * planeCount == numSatellites);
        } else if (satsPerPlane * planeCount != numSatellites) {
            EV_WARN << "TopologyControlBase: Uneven planes detected (numSatellites=" << numSatellites
                    << ", planeCount=" << planeCount << ", satsPerPlane=" << satsPerPlane
                    << "). Missing slots will be ignored." << endl;
        }
        ////VALIDATE(numGroundStations > 0);
        ///VALIDATE(numLoraDevices > 0);

        if(getSystemModule()->findSubmodule("satellite", 0) != -1){
            EV << "satellites--0"<<endl;
            loadSatellites();
        }

        if(getSystemModule()->findSubmodule("station", 0) != -1){
            EV << "groundStation--0" <<endl;
            loadGroundstations();
        }

        if(getSystemModule()->findSubmodule("terminal", 0) != -1){
            EV << "loRaNodes--0" <<endl;
            loadLoraDevices();
        }

        if (satellites.size() == 0) {
            error("Error in TopologyControl::initialize(): No satellites found.");
            return;
        }

        if (numGroundStations > 0 && groundStations.size() == 0){
            error("Error in TopologyControl::initialize(): No ground stations found.");
            return;
        }

        if (numLoraDevices > 0 && loraDevices.size() == 0){
            error("Error in TopologyControl::initialize(): No lora devices found.");
            return;
        }

        EV << "TopologyControlBase::initialize before initTopology"<<endl;
        initTopology();

        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }

    }
}

void TopologyControlBase::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updateTopology();
        scheduleUpdate();
    } else
        error("TopologyControlBase: Unknown message.");
}

void TopologyControlBase::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

void TopologyControlBase::resolveConstellationParametersFromNorad() {
    if (planeCount > 0 && interPlaneSpacing >= 0 && (!allowUnevenPlanes || satsPerPlane > 0)) {
        return;
    }

    auto* noradModule = getNoradModule();
    if (noradModule == nullptr) {
        EV_WARN << "TopologyControlBase: Unable to locate NoradModule on satellite[0] for constellation auto-configuration." << endl;
        return;
    }

    if (planeCount <= 0) {
        int derivedPlaneCount = noradModule->getNumberOfPlanes();
        if (derivedPlaneCount > 0) {
            planeCount = derivedPlaneCount;
            par("planeCount").setIntValue(planeCount);
            EV_INFO << "TopologyControlBase: Auto-configured planeCount = " << planeCount << " via NoradModule." << endl;
        }
    }

    if (interPlaneSpacing < 0) {
        int derivedSpacing = noradModule->getInterPlaneSpacing();
        if (derivedSpacing >= 0) {
            interPlaneSpacing = derivedSpacing;
            par("interPlaneSpacing").setIntValue(interPlaneSpacing);
            EV_INFO << "TopologyControlBase: Auto-configured interPlaneSpacing = " << interPlaneSpacing << " via NoradModule." << endl;
        }
    }

    if (allowUnevenPlanes && satsPerPlane <= 0) {
        int derivedSatsPerPlane = noradModule->getSatellitesPerPlane();
        if (derivedSatsPerPlane > 0) {
            satsPerPlane = derivedSatsPerPlane;
            EV_INFO << "TopologyControlBase: Auto-configured satsPerPlane = " << satsPerPlane << " via NoradModule." << endl;
        }
    }
}

Global::INorad *TopologyControlBase::getNoradModule() const {
    cModule* systemModule = getSystemModule();
    if (systemModule == nullptr) {
        return nullptr;
    }

    if (systemModule->findSubmodule("satellite", 0) == -1) {
        return nullptr;
    }

    cModule* satelliteModule = systemModule->getSubmodule("satellite", 0);
    if (satelliteModule == nullptr) {
        return nullptr;
    }

    return dynamic_cast<Global::INorad*>(satelliteModule->getSubmodule("NoradModule"));
}

Sta *const TopologyControlBase::getGroundstationInfo(int gsId) const {
    // EV << "TopologyControlBase::getGroundstationInfo: " << endl;
    // EV<<"gsId: "<<gsId<<endl;
    ASSERT(gsId >= 0 && gsId < numGroundStations);

    return groundStations.at(gsId);
}

Ter *const TopologyControlBase::getDeviceInfo(int devId) const {
    EV << "TopologyControlBase::getDeviceInfo: " << endl;
    EV<<"devId: "<<devId<<endl;
    ASSERT(devId >= 0 && devId < numLoraDevices);
    return loraDevices.at(devId);
}

Sat *const TopologyControlBase::getSatellite(int satId) const {
    ASSERT(satId >= 0 && satId < numSatellites);
    return satellites.at(satId);
}

std::unordered_map<int, Sat *> const &TopologyControlBase::getSatellites() const {
    return satellites;
}

GsSatConnection const &TopologyControlBase::getGroundstationSatConnection(int gsId, int satId) const {
    return gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

GsSatConnection const &TopologyControlBase::getDeviceSatConnection(int devId, int satId) const {
    return deviceSatConnections.at(std::pair<int, int>(devId, satId));
}

int TopologyControlBase::calculateSatelliteId(int plane, int numberInPlane) const {
    ASSERT(plane >= 0 && plane < planeCount);
    ASSERT(numberInPlane >= 0 && numberInPlane < satsPerPlane);

    int id = plane * satsPerPlane + numberInPlane;
    if (allowUnevenPlanes && (id < 0 || id >= numSatellites)) {
        return -1;
    }
    ASSERT(id >= 0 && id < numSatellites);
    return id;
}

Sat *const TopologyControlBase::findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) const {
    int id = calculateSatelliteId(plane, numberInPlane);
    if (id < 0) {
        return nullptr;
    }
    auto it = satellites.find(id);
    if (it == satellites.end()) {
        return nullptr;
    }
    return it->second;
}

void TopologyControlBase::loadGroundstations() {
    groundStations.clear();
    for (size_t i = 0; i < numGroundStations; i++) {
        Sta *gs = check_and_cast<Sta *>(getParentModule()->getSubmodule("station", i));
        EV << "TC: Loaded Groundstation " << i << endl;
        groundStations.emplace(i, gs);
    }
}

/*GroundStationRoutingBase TopologyControlBase::getGroundStation(int gsId){
    return groundStations.at(gsId);
}*/

void TopologyControlBase::loadSatellites() {
    satellites.clear();
    for (size_t i = 0; i < numSatellites; i++) {
        Sat *sat = check_and_cast<Sat *>(getParentModule()->getSubmodule("satellite", i));
        satellites.emplace(i, sat);
    }
}

void TopologyControlBase::loadLoraDevices() {
    loraDevices.clear();
    for (size_t i = 0; i < numLoraDevices; i++) {
        Ter *ld = check_and_cast<Ter *>(getSystemModule()->getSubmodule("terminal", i));
        EV << "TC: Loaded Lora Device Transmitter " << i << endl;
        loraDevices.emplace(i, ld);
    }
}

void TopologyControlBase::connectSatellites(Sat *first, Sat *second, isldirection::ISLDirection direction) {
    ASSERT(first != nullptr);
    ASSERT(second != nullptr);
    ASSERT(direction != isldirection::GROUNDLINK);

    isldirection::ISLDirection counterDirection = isldirection::getCounterDirection(direction);

    cGate *firstOut = first->getOutputGate(direction).first;
    cGate *firstIn = first->getInputGate(direction).first;
    cGate *secondOut = second->getOutputGate(counterDirection).first;
    cGate *secondIn = second->getInputGate(counterDirection).first;

    ASSERT(firstOut != nullptr);
    ASSERT(firstIn != nullptr);
    ASSERT(secondOut != nullptr);
    ASSERT(secondIn != nullptr);

#ifndef NDEBUG
    EV << "<><><><><><><><><><><><>" << endl;
    if(first->getId() == 15)
        EV << "Satellite 1555"<<endl;
    EV << "Connect/Update " << first->getId() << " and " << second->getId() << " on " << to_string(direction) << endl;
#endif

    double distance = first->getDistance(*second);
    double delay = islDelay * distance;

    // Performs the following steps for each ISL direction (excluding groundlink)
    // 1. Delete the old connections if they are not the desired ones. If disconncts are happening, signal topology change.
    // 2. Create the satellite pair connections.
    // 3. Check if it is a new connection and if yes, signal topology change.
    // 4. Save the new satellite pair.
    switch (direction) {
        case isldirection::ISLDirection::LEFT:
            // 1.
            if (second->hasRightSat() && second->getRightSatId() != first->getId()) {
                disconnectSatellites(second, second->getRightSat(), counterDirection);
            }
            if (first->hasLeftSat() && first->getLeftSatId() != second->getId()) {
                disconnectSatellites(first, first->getLeftSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getLeftSendState(), second->getRightRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getRightSendState(), first->getLeftRecvState());
            // 3.
            if (!first->hasLeftSat() || !second->hasRightSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setLeftSat(second);
            second->setRightSat(first);
            break;
        case isldirection::ISLDirection::UP:
            // 1.
            if (second->hasDownSat() && second->getDownSatId() != first->getId()) {
                disconnectSatellites(second, second->getDownSat(), counterDirection);
            }
            if (first->hasUpSat() && first->getUpSatId() != second->getId()) {
                disconnectSatellites(first, first->getUpSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getUpSendState(), second->getDownRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getDownSendState(), first->getUpRecvState());
            // 3.
            if (!first->hasUpSat() || !second->hasDownSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setUpSat(second);
            second->setDownSat(first);
            break;
        case isldirection::ISLDirection::RIGHT:
            // 1.
            if (second->hasLeftSat() && second->getLeftSatId() != first->getId()) {
                disconnectSatellites(second, second->getLeftSat(), counterDirection);
            }
            if (first->hasRightSat() && first->getRightSatId() != second->getId()) {
                disconnectSatellites(first, first->getRightSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getRightSendState(), second->getLeftRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getLeftSendState(), first->getRightRecvState());
            // 3.
            if (!first->hasRightSat() || !second->hasLeftSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setRightSat(second);
            second->setLeftSat(first);
            break;
        case isldirection::ISLDirection::DOWN:
            // 1.
            if (second->hasUpSat() && second->getUpSatId() != first->getId()) {
                disconnectSatellites(second, second->getUpSat(), counterDirection);
            }
            if (first->hasDownSat() && first->getDownSatId() != second->getId()) {
                disconnectSatellites(first, first->getDownSat(), direction);
            }
            // 2.
            createSatelliteConnection(firstOut, secondIn, delay, islDatarate, first->getDownSendState(), second->getUpRecvState());
            createSatelliteConnection(secondOut, firstIn, delay, islDatarate, second->getUpSendState(), first->getDownRecvState());
            // 3.
            if (!first->hasDownSat() || !second->hasUpSat()) {
                topologyChanged = true;
            }
            // 4.
            first->setDownSat(second);
            second->setUpSat(first);
            break;
        default:
            error("Error in TopologyControlBase::connectSatellites: Should not reach default branch of switch.");
            break;
    }
}

void TopologyControlBase::disconnectSatellites(Sat *first, Sat *second, isldirection::ISLDirection direction) {
    ASSERT(direction != isldirection::GROUNDLINK);

#ifndef NDEBUG
    EV << "Disconnect " << first->getId() << " and " << second->getId() << " on " << to_string(direction) << endl;
#endif

    cGate *firstOut = first->getOutputGate(direction).first;
    cGate *secondOut = second->getOutputGate(isldirection::getCounterDirection(direction)).first;

    ASSERT(firstOut != nullptr);
    ASSERT(secondOut != nullptr);

    firstOut->disconnect();
    secondOut->disconnect();

    topologyChanged = true;

    switch (direction) {
        case isldirection::ISLDirection::LEFT:
            first->removeLeftSat();
            second->removeRightSat();
            break;
        case isldirection::ISLDirection::UP:
            first->removeUpSat();
            second->removeDownSat();
            break;
        case isldirection::ISLDirection::RIGHT:
            first->removeRightSat();
            second->removeLeftSat();
            break;
        case isldirection::ISLDirection::DOWN:
            first->removeDownSat();
            second->removeUpSat();
            break;
        default:
            error("Error in TopologyControlBase::disconnectSatellites: Should not reach default branch of switch.");
            break;
    }
}

void TopologyControlBase::createSatelliteConnection(cGate *outGate, cGate *inGate, double delay, double datarate, ISLState outState, ISLState inState) {
    if (outState == ISLState::WORKING && inState == ISLState::WORKING) {
        updateOrCreateChannel(outGate, inGate, delay, islDatarate);
    } else {
        outGate->disconnect();
    }
}

ChannelState TopologyControlBase::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    ASSERT(outGate != nullptr);
    ASSERT(inGate != nullptr);
    ASSERT(delay > 0.0);
    ASSERT(datarate > 0.0);

    cGate *nextGate = outGate->getNextGate();
    if (nextGate != nullptr && nextGate->getId() == inGate->getId()) {
        ASSERT(outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Update channel: " << ((Sat *)outGate->getOwnerModule())->getId() << "->" << ((Sat *)inGate->getOwnerModule())->getId() << endl;
#endif
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        ASSERT(!outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Create channel: " << ((Sat *)outGate->getOwnerModule())->getId() << "->" << ((Sat *)inGate->getOwnerModule())->getId() << endl;
#endif

        cDatarateChannel *channel = cDatarateChannel::create(Constants::ISL_CHANNEL_NAME);
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        outGate->connectTo(inGate, channel);
        channel->callInitialize();

        return ChannelState::CREATED;
    }
}

ChannelState TopologyControlBase::deleteChannel(cGate *outGate) {
    if (outGate != nullptr && outGate->isConnectedOutside()) {
        outGate->disconnect();
        return ChannelState::DELETED;
    }
    return ChannelState::UNCHANGED;
}

}  // namespace topologycontrol

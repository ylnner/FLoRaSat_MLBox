/*
 * Sat.cc
 *
 *  Created on: Oct 14, 2025
 *      Author: root
 */


#include "Sat.h"
using namespace Global;

namespace satellite {

Define_Module(Sat);

void Sat::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        // Navigate to parent module if this is a submodule, otherwise use this module
        cModule* satelliteModule = getParentModule() != nullptr && strcmp(getParentModule()->getNedTypeName(), "Nodes._20_Satellite.Satellite") == 0
            ? getParentModule()
            : this;
        satId = satelliteModule->getIndex();
        lowerLatitudeBound = satelliteModule->par("lowerLatitudeBound");
        upperLatitudeBound = satelliteModule->par("upperLatitudeBound");

        ASSERT(lowerLatitudeBound >= -90 && lowerLatitudeBound <= 90);
        ASSERT(upperLatitudeBound >= -90 && upperLatitudeBound <= 90);
        ASSERT(upperLatitudeBound >= lowerLatitudeBound);
    } else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {
        // Get noradModule in a later stage to ensure ConstellationCreator has
        // finished replacing the NoradModule submodule
        cModule* satelliteModule = getParentModule() != nullptr && strcmp(getParentModule()->getNedTypeName(), "Nodes._20_Satellite.Satellite") == 0
            ? getParentModule()
            : this;
        //noradModule = check_and_cast<INorad*>(satelliteModule->getSubmodule("NoradModule"));
        noradModule = check_and_cast<Sat_Mob_NoradA*>(satelliteModule->getSubmodule("NoradModule"));
    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        // calculate plane
        int satsPerPlane = noradModule->getSatellitesPerPlane();
        satPlane = (satId - (satId % satsPerPlane)) / satsPerPlane;
        // ASSERT(satPlane < noradModule->getNumberOfPlanes());
        ASSERT(satPlane >= 0);

        // calculate number in plane
        satNumberInPlane = satId % satsPerPlane;
        ASSERT(satNumberInPlane >= 0);
        ASSERT(satNumberInPlane < satsPerPlane);
    }
}

void Sat::finish() {
}

#ifndef NDEBUG
void Sat::refreshDisplay() const {
    char buf[40];

    int leftId = -1;
    if (hasLeftSat()) {
        leftId = getLeftSatId();
    }
    int upId = -1;
    if (hasUpSat()) {
        upId = getUpSatId();
    }
    int rightId = -1;
    if (hasRightSat()) {
        rightId = getRightSatId();
    }
    int downId = -1;
    if (hasDownSat()) {
        downId = getDownSatId();
    }
    sprintf(buf, "left: %d up: %d right: %d down: %d", leftId, upId, rightId, downId);
    getDisplayString().setTagArg("t", 0, buf);
}
#endif

std::pair<cGate*, ISLState> Sat::getInputGate(isldirection::ISLDirection direction, int index) {
    return getGate(direction, cGate::Type::INPUT, index);
}

std::pair<cGate*, ISLState> Sat::getOutputGate(isldirection::ISLDirection direction, int index) {
    return getGate(direction, cGate::Type::OUTPUT, index);
}

std::pair<cGate*, ISLState> Sat::getGate(isldirection::ISLDirection direction, cGate::Type type, int index) {

    //cModule *netModule = getSubmodule("net");
    cGate* gatePtr;
    ISLState state;
    switch (direction) {
        case isldirection::ISLDirection::LEFT: {
            if (type == cGate::Type::OUTPUT) {
                gatePtr = gate(Constants::ISL_LEFT_NAME_OUT);
                state = getLeftSendState();
            } else {
                gatePtr = gate(Constants::ISL_LEFT_NAME_IN);
                state = getLeftRecvState();
            }
        } break;

        case isldirection::ISLDirection::UP: {
            if (type == cGate::Type::OUTPUT) {
                gatePtr = gate(Constants::ISL_UP_NAME_OUT);
                state = getUpSendState();
            } else {
                gatePtr = gate(Constants::ISL_UP_NAME_IN);
                state = getUpRecvState();
            }
        } break;
        case isldirection::ISLDirection::RIGHT: {
            if (type == cGate::Type::OUTPUT) {
                gatePtr = gate(Constants::ISL_RIGHT_NAME_OUT);
                state = getRightSendState();
            } else {
                gatePtr = gate(Constants::ISL_RIGHT_NAME_IN);
                state = getRightRecvState();
            }
        } break;
        case isldirection::ISLDirection::DOWN: {
            if (type == cGate::Type::OUTPUT) {
                gatePtr = gate(Constants::ISL_DOWN_NAME_OUT);
                state = getDownSendState();
            } else {
                gatePtr = gate(Constants::ISL_DOWN_NAME_IN);
                state = getDownRecvState();
            }
        } break;
        case isldirection::ISLDirection::GROUNDLINK: {
            if (type == cGate::Type::OUTPUT) {
                gatePtr = gate(Constants::SAT_GROUNDLINK_NAME_OUT, index);
            } else {
                gatePtr = gate(Constants::SAT_GROUNDLINK_NAME_IN, index);
            }
            state = ISLState::WORKING;
        } break;
        default:
            error("Unexpected gate");
    }
    return std::make_pair(gatePtr, state);
}

bool Sat::hasLeftSat() const {
    return leftSatellite != nullptr;
}
bool Sat::hasUpSat() const {
    return upSatellite != nullptr;
}
bool Sat::hasRightSat() const {
    return rightSatellite != nullptr;
}
bool Sat::hasDownSat() const {
    return downSatellite != nullptr;
}

void Sat::setLeftSat(Sat* newSat) {
    EV << "satId: "<< satId <<endl;
    ASSERT(newSat != nullptr);
    leftSatellite = newSat;
}
void Sat::setUpSat(Sat* newSat) {
    EV << "satId: "<< satId <<endl;
    ASSERT(newSat != nullptr);
    upSatellite = newSat;
}
void Sat::setRightSat(Sat* newSat) {
    EV << "satId: "<< satId <<endl;
    ASSERT(newSat != nullptr);
    rightSatellite = newSat;
}
void Sat::setDownSat(Sat* newSat) {
    EV << "satId: "<< satId <<endl;
    ASSERT(newSat != nullptr);
    downSatellite = newSat;
}

void Sat::removeLeftSat() {
    ASSERT(leftSatellite != nullptr);
    leftSatellite = nullptr;
}
void Sat::removeUpSat() {
    ASSERT(upSatellite != nullptr);
    upSatellite = nullptr;
}
void Sat::removeRightSat() {
    ASSERT(rightSatellite != nullptr);
    rightSatellite = nullptr;
}
void Sat::removeDownSat() {
    ASSERT(downSatellite != nullptr);
    downSatellite = nullptr;
}

Sat* Sat::getLeftSat() const {
    ASSERT(hasLeftSat());
    return leftSatellite;
}
Sat* Sat::getUpSat() const {
    ASSERT(hasUpSat());
    return upSatellite;
}
Sat* Sat::getRightSat() const {
    ASSERT(hasRightSat());
    return rightSatellite;
}
Sat* Sat::getDownSat() const {
    ASSERT(hasDownSat());
    return downSatellite;
}

int Sat::getLeftSatId() const {
    ASSERT(hasLeftSat());
    return leftSatellite->getId();
}
int Sat::getUpSatId() const {
    ASSERT(hasUpSat());
    return upSatellite->getId();
}
int Sat::getRightSatId() const {
    ASSERT(hasRightSat());
    return rightSatellite->getId();
}
int Sat::getDownSatId() const {
    ASSERT(hasDownSat());
    return downSatellite->getId();
}

double Sat::getLeftSatDistance() const {
    ASSERT(hasLeftSat());
    return leftSatellite->getDistance(*this);
}
double Sat::getUpSatDistance() const {
    ASSERT(hasUpSat());
    return upSatellite->getDistance(*this);
}
double Sat::getRightSatDistance() const {
    ASSERT(hasRightSat());
    return rightSatellite->getDistance(*this);
}
double Sat::getDownSatDistance() const {
    ASSERT(hasDownSat());
    return downSatellite->getDistance(*this);
}

void Sat::setLeftSendState(ISLState newState) {
    leftSendState = newState;
}
void Sat::setLeftRecvState(ISLState newState) {
    leftRecvState = newState;
}
void Sat::setUpSendState(ISLState newState) {
    upSendState = newState;
}
void Sat::setUpRecvState(ISLState newState) {
    upRecvState = newState;
}
void Sat::setRightSendState(ISLState newState) {
    rightSendState = newState;
}
void Sat::setRightRecvState(ISLState newState) {
    rightRecvState = newState;
}
void Sat::setDownSendState(ISLState newState) {
    downSendState = newState;
}
void Sat::setDownRecvState(ISLState newState) {
    downRecvState = newState;
}

void Sat::setISLSendState(isldirection::ISLDirection direction, ISLState state) {
    setISLState(direction, true, state);
}
void Sat::setISLRecvState(isldirection::ISLDirection direction, ISLState state) {
    setISLState(direction, false, state);
}
void Sat::setISLState(isldirection::ISLDirection direction, bool send, ISLState state) {
    EV << "[" << satId << "] Set " << to_string(direction) << " to " << to_string(state) << endl;
    switch (direction) {
        case isldirection::LEFT:
            send ? setLeftSendState(state) : setLeftRecvState(state);
            break;
        case isldirection::UP:
            send ? setUpSendState(state) : setUpRecvState(state);
            break;
        case isldirection::RIGHT:
            send ? setRightSendState(state) : setRightRecvState(state);
            break;
        case isldirection::DOWN:
            send ? setDownSendState(state) : setDownRecvState(state);
            break;
        case isldirection::GROUNDLINK:
            error("Groundlink is not supported.");
        default:
            break;
    }
}

double Sat::getLongitude() const {
    return noradModule->getLongitude();
}
double Sat::getLatitude() const {
    return noradModule->getLatitude();


}
double Sat::getAltitude() const {
    return noradModule->getAltitude();
}

double Sat::getNumberOfPlanes() const {
    return noradModule->getNumberOfPlanes();
}
double Sat::getSatsPerPlane() const {
    return noradModule->getSatellitesPerPlane();
}
double Sat::getRAAN() const {
    return rad2deg(noradModule->getRaan());
}

double Sat::getMnAnomaly() const {
    return rad2deg(noradModule->getMnAnomaly());
}

double Sat::getElevation(const PositionAwareBase& other) {
    // Update satellite position to current simulation time before calculating elevation
    noradModule->updateTime(simTime());
    double elev = ((INorad*)noradModule)->getElevation(other.getLatitude(), other.getLongitude(), other.getAltitude());
    EV_DEBUG << "Sat " << satId << " elevation to (" << other.getLatitude() << ", " << other.getLongitude() << "): " << elev << " deg at t=" << simTime() << endl;
    EV << "Sat " << satId << " elevation to (" << other.getLatitude() << ", " << other.getLongitude() << "): " << elev << " deg at t=" << simTime() << endl;
    return elev;
}

double Sat::getFutureElevation(const PositionAwareBase& other, simtime_t offset) const {
    return noradModule->getFutureElevation(offset, other.getLatitude(), other.getLongitude(), other.getAltitude());
}

double Sat::getAzimuth(const PositionAwareBase& other) const {
    return ((INorad*)noradModule)->getAzimuth(other.getLatitude(), other.getLongitude(), other.getAltitude());
}

bool Sat::isAscending() const {
    return noradModule->isAscending();
}

bool Sat::isDescending() const {
    return !noradModule->isAscending();
}

bool Sat::isInterPlaneISLEnabled() const {
    double latitude = getLatitude();
    return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
}

std::string to_string(const Sat& satRoutingBase) {
    std::ostringstream ss;
    ss << satRoutingBase;
    return ss.str();
}


void Sat::printConnectedSatellites(){
    EV << "Sat::printConnectedSatellites"<<endl;
    EV << "Current Sat: " << satId <<endl;

    EV << "leftSatellite: " << leftSatellite->getId() << endl;
    EV << "rightSatellite: " << rightSatellite->getId()<< endl;
    EV << "upSatellite: " << upSatellite->getId()<< endl;
    EV << "downSatellite: " << downSatellite->getId() <<  endl;
}


void Sat::handleMessage(cMessage *msg){
    EV << "Sat::handleMessage" <<endl;
    Packet *pkt = check_and_cast<Packet *>(msg);
    EV << "Arrival gate: " << msg->getArrivalGate()->getName() <<endl;
    EV << "index gate: " << msg->getArrivalGate()-> getIndex() <<endl;
}

}  // namespace satellite


/*
 * Sta_Net.cc
 *
 *  Created on: Oct 24, 2025
 *      Author: root
 */


#include "Sta_Net.h"

Define_Module(Sta_Net);

void Sta_Net::initialize(int stage) {

    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getParentModule()->getIndex();
        mobility = check_and_cast<Sta_Mob_StationMobility*>(getParentModule()->getSubmodule("mobility"));
        satellites = std::set<int>();
        sendData = par("sendData");
        dstGsOrDev = par("dstGsOrDev");
    }
}

cGate* Sta_Net::getInputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* Sta_Net::getOutputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double Sta_Net::getLongitude() const {
    return mobility->getLongitude();
}

double Sta_Net::getLatitude() const {
    return mobility->getLatitude();
}

double Sta_Net::getAltitude() const {
    return 0;
}

const std::set<int>& Sta_Net::getSatellites() const {
    return satellites;
}

void Sta_Net::addSatellite(int satId) {
    ASSERT(!base::set::contains(satellites, satId));
    satellites.emplace(satId);
}

void Sta_Net::removeSatellite(int satId) {
    ASSERT(base::set::contains(satellites, satId));
    satellites.erase(satId);
}

bool Sta_Net::isConnectedToAnySat() {
    return !satellites.empty();
}

bool Sta_Net::isConnectedTo(int satId) {
    return base::set::contains(satellites, satId);
}

void Sta_Net::handleMessage(cMessage *msg) {
    EV_INFO << "Message " << msg->getName() << " received at GroundStationRoutingBase ["
            << groundStationId << "]" << endl;

    delete msg;
}

std::string to_string(const Sta_Net& gs) {
    std::ostringstream ss;
    ss << gs;
    return ss.str();
}


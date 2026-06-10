/*
 * Ter.cc
 *
 *  Created on: Oct 25, 2025
 *      Author: root
 */

#include "Ter.h"
using namespace terminal;
using namespace omnetpp;
using namespace base;
using namespace mobility;
using namespace inet;

namespace terminal{
Define_Module(Ter);

void Ter::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL){
        loraNodeId = getIndex();
        satellites = std::set<int>();

        numGroundStations = getSystemModule()->getSubmoduleVectorSize("station");
        numLoraNodes      = getSystemModule()->getSubmoduleVectorSize("terminal");
        /////maxHops           = par("maxHops").intValue();
        // cached ptrs
        mobility        = check_and_cast<mobility::Ter_Mob *>(getSubmodule("mob"));
        dstGsOrDev = par("dstGsOrDev");

    }
}


cGate* Ter::getInputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* Ter::getOutputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double Ter::getLongitude() const{
    return mobility->getLongitude();
}

double Ter::getLatitude() const {
    return mobility->getLatitude();
}

double Ter::getAltitude() const{
    return 0;
}

const std::set<int>& Ter::getSatellites() const {
    return satellites;
}

void Ter::addSatellite(int satId) {
    ASSERT(!set::contains(satellites, satId));
    satellites.emplace(satId);
}

void Ter::removeSatellite(int satId) {
    ASSERT(set::contains(satellites, satId));
    satellites.erase(satId);
}

bool Ter::isConnectedToAnySat() {
    return !satellites.empty();
}

bool Ter::isConnectedTo(int satId) {
    return set::contains(satellites, satId);
}

void Ter::handleMessage(cMessage *msg) {
    EV_INFO << "Message " << msg->getName() << " received at terminal ["
            << loraNodeId << "]" << endl;

    delete msg;
}

std::string to_string(const Ter& ln) {
    std::ostringstream ss;
    ss << ln;
    return ss.str();
}

}

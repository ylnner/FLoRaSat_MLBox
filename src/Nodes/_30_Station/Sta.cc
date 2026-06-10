#include "Sta.h"
//using namespace station;
using namespace mobility;
using namespace core;
using namespace inet;

namespace station{

Define_Module(Sta);

void Sta::initialize(int stage) {

    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getIndex();
        mobility = check_and_cast<Sta_Mob_StationMobility*>(getSubmodule("mobility"));
        net = check_and_cast<Sta_Net*>(getSubmodule("net"));
        phy = check_and_cast<cSimpleModule*>(getSubmodule("phy"));
        satellites = std::set<int>();
        sendData = par("sendData");
        dstGsOrDev = par("dstGsOrDev");
    }
}

cGate* Sta::getInputGate(int index) {
    //return net->gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* Sta::getOutputGate(int index) {
    //return net->gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double Sta::getLongitude() const {
    return mobility->getLongitude();
}

double Sta::getLatitude() const {
    return mobility->getLatitude();
}

double Sta::getAltitude() const {
    return 0;
}

const std::set<int>& Sta::getSatellites() const {
    return satellites;
}

void Sta::addSatellite(int satId) {
    ASSERT(!base::set::contains(satellites, satId));
    satellites.emplace(satId);
}

void Sta::removeSatellite(int satId) {
    ASSERT(base::set::contains(satellites, satId));
    satellites.erase(satId);
}

bool Sta::isConnectedToAnySat() {
    return !satellites.empty();
}

bool Sta::isConnectedTo(int satId) {
    return base::set::contains(satellites, satId);
}

std::string to_string(const Sta& gs) {
    std::ostringstream ss;
    ss << gs;
    return ss.str();
}
}

/*
 * Base_Routing.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "Base_Routing.h"

using namespace base::set;
using namespace base;

namespace routing {
Define_Module(Base_Routing);

void Base_Routing::initialize(int stage){

    if (stage == inet::INITSTAGE_LOCAL) {
        topologyControl = check_and_cast<topologycontrol::TopologyControlBase*>(getSystemModule()->getSubmodule("topologyControl"));
        sat = check_and_cast<Sat*>(getParentModule()->getParentModule());
        //Sat_Net_PacketHandler
        sender = check_and_cast<Sat_Net_PacketHandler*>(getParentModule()->getSubmodule("packetHandler"));

        satId = sat->getId();
    }

}

void Base_Routing::handlePacket(inet::Packet *pkt) {
    // Implementación base (puede estar vacía o lanzar un error)
    EV_DEBUG << "Base_Routing: not implemeted." << std::endl;
}

void Base_Routing::handleMaxHopsReached(inet::Packet *pkt) {
    EV_DEBUG << "Dropping: " << pkt << " has reached max hops." << endl;
    sender->dropPacket(pkt, PacketDropReason::HOP_LIMIT_REACHED, false);

}

int Base_Routing::getGroundlinkIndex(int satelliteId, int groundstationId, int dstGsOrDev) {
    // EV << "RoutingBase::getGroundlinkIndex" <<endl;
    //EV << "dstGsOrDev: " << dstGsOrDev <<endl;
    //EV << "satelliteId: " << satelliteId << " groundstationId: "<< groundstationId <<endl;
    std::set<int> gsSatellites = getConnectedSatellites(groundstationId, dstGsOrDev);

    if (contains(gsSatellites, satelliteId)) {
        if(dstGsOrDev == 1){
            EV << "Dst is Dev - 1" <<endl;
            return topologyControl->getDeviceSatConnection(groundstationId, satelliteId).satGateIndex;
        }else if (dstGsOrDev == 0){
            EV << "Dst is GS - 0" <<endl;
            return topologyControl->getGroundstationSatConnection(groundstationId, satelliteId).satGateIndex;
        }else {
            EV << "Should not reach this point in getGroundlinkIndex"<<endl;
        }
    }

    return -1;
}

inet::Packet* Base_Routing::createControlPacketBase(const char *name, int dstSat, int dstGs, int ttl) {
    auto pkt = new Packet(name);

    pkt->addTagIfAbsent<inet::CreationTimeTag>();
    pkt->addTagIfAbsent<inet::QueueingTimeTag>();
    pkt->addTagIfAbsent<inet::ProcessingTimeTag>();
    pkt->addTagIfAbsent<inet::TransmissionTimeTag>();
    pkt->addTagIfAbsent<inet::PropagationTimeTag>();

    int64_t now = getMillisSinceEpoch();
    pkt->addTag<Net_ProcessPacketTag>()->setTime(now);

    // create constellation specific tag, required for hop handling and statistic recording
    auto routingTag = pkt->addTagIfAbsent<routing::CstRoutingTag>();
    routingTag->setType(routing::CstPacketType::CONTROL);
    routingTag->setSrcSat(satId);
    routingTag->setDstSat(dstSat);
    routingTag->setDstGs(dstGs);
    routingTag->setMaxHops(ttl);
    routing::Hop hop = routing::Hop();
    hop.type = routing::HopType::SAT;
    hop.id = satId;
    hop.lat = sat->getLatitude();
    hop.lon = sat->getLongitude();
    hop.alt = (int)round(sat->getAltitude());
    routingTag->appendRoute(hop);
    return pkt;
}

const std::set<int>& Base_Routing::getConnectedSatellites(int groundStationId, int dstGsOrDev) const {
    if (topologyControl == nullptr) error("Error in RoutingBase::GetGroundStationConnections(): topologyControl is nullptr. Did you call initialize on RoutingBase?");

    if( dstGsOrDev == 1){
        // EV << "Prepare to get sats connected to device: " << groundStationId<<endl;
        return topologyControl->getDeviceInfo(groundStationId)->getSatellites();
    }else if (dstGsOrDev == 0){
        // EV << "Prepare to get sats connected to ground station: " << groundStationId <<endl;
        return topologyControl->getGroundstationInfo(groundStationId)->getSatellites();
    }else{
        EV << "Should not reach this point." <<endl;
    }
}


}  // namespace routing

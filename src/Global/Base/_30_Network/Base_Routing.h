/*
 * Base_Routing.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_Base_Routing_H_
#define __FLORA_ROUTING_Base_Routing_H_

#include <omnetpp.h>

#include <vector>

#include "Global/Utilities/Utils.h"
#include "Global/Utilities/utils/VectorUtils.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Packet.h"

#include "Nodes/_30_Station/Sta.h"
#include "Nodes/_10_Terminal/Ter.h"
#include "Nodes/_20_Satellite/Sat.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketSender.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketHandler.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Global/Utilities/Routing_Core/DijkstraShortestPath.h"
#include "Global/Utilities/Routing_Core/UnroutableException.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketSender.h"
#include "Global/Messages/_30_Network/Net_ProcessPacketTag_m.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_SatelliteRouting.h"
#include "Global/Topology_Control/TopologyControlBase.h"
#include "Global/Base/_20_Data_Link/Base_MacType.h"

namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace isldirection;
using namespace topologycontrol;

class Base_Routing : public cSimpleModule{

protected:
    // RoutingBase
    topologycontrol::TopologyControlBase *topologyControl = nullptr;

    Sat *sat = nullptr;
    Sat_Net_PacketSender *sender = nullptr;

    int satId = -1;

    // ACHF
    int dstGsOrDev = 0;
    int srcGsOrDev = 1;

public:
    /**
     * Handler that is called on all pre-plannable topology changes.
     * E.g., is not called if ISL connection does no longer work.
     * Can be used to simulate preplanned data.
     *
     * -> Base implementation is intentionally blank.
     *
     * @param topologyChanges indicates whether a time slice has ended or actual changes to topology occurred.
     *
     */
    virtual void handleTopologyChange(bool topologyChanges){};
    /*
    * Called after satellite received the packet and gets the routing data from header
    * */
    // virtual void getRoutingDataFromHeaders(inet::Packet *pkt){};
    /**
     * Called after satellite received packet from ground station.
     */
    virtual void preparePacket(inet::Packet *pkt){};

    /**
     * Called before satellite sends packet to ground station.
     */
    virtual void wrapUpPacket(inet::Packet *pkt){};

    /**
     * Handler for packets and messages exchanged between satellites to organize congestion control, etc.
     */
    virtual void handlePacket(inet::Packet *pkt); // = 0;

    /**
     * Handler called if the packet hops has reached 0.
     */
    virtual void handleMaxHopsReached(inet::Packet *pkt);

    /**
     * Handler called if the packet handler drops a packet.
     *
     * -> Base implementation is intentionally blank, as not all algorithms are dynamic.
     */
    virtual void handlePacketDrop(inet::Packet *pkt, inet::PacketDropReason reason){};

    /**
     * Handler called if satellites enqueues a packet. Is called with the current queueSize.
     *
     * -> Base implementation is intentionally blank, as not all algorithms are dynamic.
     */
    virtual void handleQueueSize(int queueSize, int maxQueueSize){};

    /** @brief Returns the index of a groundlink inside the groundlink gate vector.*/
    int getGroundlinkIndex(int satelliteId, int groundstationId, int dstGsOrDev = 0);

    /**
     * Creates the base of a packet used to control protocol orchestration.
     */
    inet::Packet *createControlPacketBase(const char *name, int dstSat, int dstGs, int ttl = 1);

protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    /**
    * Handler for omnetpp self messages.
    *
    * -> Base implementation is not required and unimplemented.
    */
    virtual void handleMessage(cMessage *msg) override {
     error("Unimplemented");
    };

    std::set<int> const &getConnectedSatellites(int groundStationId, int dstGsOrDev = 1) const;
};

}  // namespace routing

#endif  // __FLORA_ROUTING_Base_Routing_H_

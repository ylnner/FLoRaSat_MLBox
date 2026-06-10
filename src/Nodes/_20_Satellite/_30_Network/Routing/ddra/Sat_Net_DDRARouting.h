/*
 * Sat_Net_DDRARouting.h
 *
 *  Created on: Jan 20, 2026
 *      Author: ylnner
 */

#ifndef NODES__20_SATELLITE__30_NETWORK_SAT_NET_DDRAROUTING_H_
#define NODES__20_SATELLITE__30_NETWORK_SAT_NET_DDRAROUTING_H_


#include <vector>
#include <string>
#include "inet/common/packet/Packet.h"
#include "Global/Utilities/Utils.h"
#include "Global/Utilities/utils/VectorUtils.h"
#include "Global/Messages/_30_Network/DDRARoutingHeader_m.h"
#include "Global/Messages/_30_Network/SendOnTag_m.h"
#include "Global/Base/_30_Network/Base_Routing.h"
#include "omnetpp.h"
#include "Global/Base/_30_Network/FailedLink.h"
#include "Global/Topology_Control/ConstellationTopologyControl.h"
#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"
#include "Nodes/_10_Terminal/_30_Network/routing/ddra/Ter_Net_DDRARouting.h"

using namespace topologycontrol;
using namespace omnetpp;
//using namespace routing::ddra;

namespace routing {

struct Node {
    bool visited = false;
    int distance = INT_MAX;
    int prev = -1;
};

class Sat_Net_DDRARouting : public Base_Routing, public ForwardingTable{

// DDRARouting
   protected:
    int broadcastId = 0;
    int queueThreshold = -1;

    // state
    int leftDrops = 0;
    int upDrops = 0;
    int rightDrops = 0;
    int downDrops = 0;

    // dynamic state
    bool congestedMsgSent = false;
    std::vector<std::vector<int>> costMatrix;
    std::set<int> congestedSats;
    std::vector<FailedLink> failedLinks;
    std::set<int> knownUnroutableSats;

   public:
    virtual void handleTopologyChange(bool topologyChanges) override;
    virtual void handlePacket(inet::Packet *pkt) override;
    virtual void handlePacketDrop(inet::Packet *pkt, inet::PacketDropReason reason) override;
    virtual void handleQueueSize(int queueSize, int maxQueueSize) override;
    virtual void handleMaxHopsReached(inet::Packet *pkt) override;

   protected:
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void createControlPacket(ddra::PacketType type, int failureTargetSat = -1);
    void handleControlPacket(inet::Ptr<const routing::ddra::DDRARoutingHeader> header);
    // void broadcastPacket(inet::Packet *pkt);

    void applyDynamicRoutes();

    void recalculateRoutes();
    virtual void finish() override;
};

}


#endif /* NODES__20_SATELLITE__30_NETWORK_SAT_NET_DDRAROUTING_H_ */

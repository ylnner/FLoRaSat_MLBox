/*
 * Ter_Base_Routing.h
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_BASE__30_NETWORK_TER_BASE_ROUTING_H_
#define GLOBAL_BASE__30_NETWORK_TER_BASE_ROUTING_H_

#include <omnetpp.h>

#include <vector>

#include "Global/Utilities/Utils.h"
#include "Global/Utilities/utils/VectorUtils.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Packet.h"

#include "Nodes/_10_Terminal/Ter.h"
#include "Nodes/_10_Terminal/_30_Network/GroundForwardingTable.h"

#include "Nodes/_20_Satellite/Sat.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketSender.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketHandler.h"
#include "Nodes/_20_Satellite/_30_Network/ForwardingTable.h"

#include "Nodes/_30_Station/Sta.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Global/Utilities/Routing_Core/DijkstraShortestPath.h"
#include "Global/Utilities/Routing_Core/UnroutableException.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketSender.h"
#include "Global/Messages/_30_Network/Net_ProcessPacketTag_m.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_SatelliteRouting.h"
#include "Global/Topology_Control/TopologyControlBase.h"
#include "Global/Base/_20_Data_Link/Base_MacType.h"

#include "Nodes/_10_Terminal/_30_Network/Ter_Net_TerminalRouting.h"

namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace isldirection;
using namespace topologycontrol;

class Ter_Base_Routing : public cSimpleModule, public GroundForwardingTable {

protected:
    // RoutingBaseGS
    topologycontrol::TopologyControlBase *topologyControl = nullptr;

    // Tables to destination (Ground and Sat)
    //ForwardingTable *satRoutingTable = nullptr;
    //GroundForwardingTable *groundRoutingTable = nullptr;

    Sta *gs = nullptr;
    Ter *ld = nullptr;
    bool isTerminal = false; // It's lora node

    int id = -1;

public:
    int getId(){return id;}
    virtual int getClosestDstId(int dstGsOrDev);
    /////virtual std::pair<int, int> calculateFirstAndLastSatellite(int dstGs);
    /////virtual std::pair<int, int> calculateFirstAndLastSatelliteToDevice(int dstDev);

    /**
    * Called if a normal packet arrives from a satellite.
    * Can be used to remove headers.
    */
    virtual void receivePacket(inet::Packet *pkt){};

    /**
    * Called before a normal packet is send by satellite.
    * Can be used to add headers.
    */
    virtual void prepareSendPacket(inet::Packet *pkt, int firstSat, int lastSat, int dstGs){EV << "RoutingBaseGS::prepareSendPacket"<<endl;};

    /**
    * Called if a control packet arrives from a satellite.
    * Can be used for congestion control and similar.
    */
    virtual void handlePacket(inet::Packet *pkt){};

    /**
    * Handler that is called on all pre-plannable topology changes.
    * E.g., is not called if ISL connection does no longer work.
    * Can be used to simulate preplanned data.
    *
    * @param topologyChanges indicates whether a time slice has ended or actual changes to topology occurred.
    *
    */
    virtual void handleTopologyChange(bool topologyChanges);
    virtual void handleTopologyChangeDevices(bool topologyChanges);

protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    /**
    * Handler for omnetpp self messages.
    */
    virtual void handleMessage(cMessage *msg) override {
     error("Unsupported");
    }

private:
    void recalculateRoutes();
    void recalculateRoutesDevices();

};
}// namespace routing

#endif /* GLOBAL_BASE__30_NETWORK_TER_BASE_ROUTING_H_ */

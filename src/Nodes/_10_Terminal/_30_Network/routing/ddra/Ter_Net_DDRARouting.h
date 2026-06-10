/*
 * Ter_Net_DDRARouting.h
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__30_NETWORK_ROUTING_DDRA_TER_NET_DDRAROUTING_H_
#define NODES__10_TERMINAL__30_NETWORK_ROUTING_DDRA_TER_NET_DDRAROUTING_H_

#include <vector>
#include <string>
#include "inet/common/packet/Packet.h"
#include "Global/Utilities/Utils.h"
#include "Global/Utilities/utils/VectorUtils.h"
#include "Global/Messages/_30_Network/DDRARoutingHeader_m.h"
#include "Global/Messages/_30_Network/SendOnTag_m.h"
#include "Global/Base/_30_Network/Ter_Base_Routing.h"
#include "omnetpp.h"
#include "Global/Base/_30_Network/FailedLink.h"
#include "Global/Topology_Control/ConstellationTopologyControl.h"

using namespace topologycontrol;
using namespace omnetpp;
using namespace routing;

namespace routing{

class Ter_Net_DDRARouting : public Ter_Base_Routing {

protected:
    std::vector<std::vector<int>> costMatrix;
    std::set<int> congestedSats;
    std::vector<FailedLink> failedLinks;
    std::set<int> knownUnroutableSats;


public:
    ////virtual std::pair<int, int> calculateFirstAndLastSatellite(int dstGs) override;
    ////virtual std::pair<int, int> calculateFirstAndLastSatelliteToDevice(int dstDev) override;
    virtual void handlePacket(inet::Packet *pkt) override;
    virtual void prepareSendPacket(inet::Packet *pkt, int firstSat, int lastSat, int dstGs) override;
    virtual void receivePacket(inet::Packet *pkt) override;
    virtual void handleTopologyChange(bool topologyChanges) override;
    virtual void handleTopologyChangeDevices(bool topologyChanges);

protected:
    virtual void initialize(int stage) override;
    void recalculateRoutes();
    void recalculateRoutesDevices();
};

}



#endif /* NODES__10_TERMINAL__30_NETWORK_ROUTING_DDRA_TER_NET_DDRAROUTING_H_ */

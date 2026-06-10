/*
 * ConstellationTopologyControl.h
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */

#ifndef GLOBAL_TOPOLOGY_CONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_
#define GLOBAL_TOPOLOGY_CONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_

#include "TopologyControlBase.h"
#include "Global/Utilities/Timer.h"

#include "TopologyControlBase.h"
//#include "LoRaApp/SimpleLoRaApp.h"
#include "Nodes/_10_Terminal/Ter.h"
//#include "routing/core/DijkstraShortestPath.h"
#include "Global/Utilities/Routing_Core/DijkstraShortestPath.h"
//#include "Nodes/_20_Satellite/_30_Network/Sat_Net_RoutingBase.h"
//#include "core/Timer.h"
#include "Global/Base/_30_Network/Base_Routing.h"
#include "Global/Base/_30_Network/Ter_Base_Routing.h"
//#include "routing/RoutingBaseGs.h"

using namespace routing;

namespace topologycontrol {
class ConstellationTopologyControl : public TopologyControlBase{
    public:
        bool isStillConnectedAt(int gsId, int satId, simtime_t offset, bool isGS = true) override;
        const std::vector<std::vector<int>> &getCostMatrix() {
            EV << "getCostMatrix"<<endl;
            return costMatrix;
        }
    protected:
        ~ConstellationTopologyControl();
        virtual void initialize(int stage) override;
        void handleMessage(cMessage *msg) override;
        virtual void initTopology() override;
        virtual void updateTopology() override;
        virtual void trackTopologyChange() override;

        double timeSliceInterval            = -1;
        ClockEvent *timeSliceTimer          = nullptr;
        simtime_t lastChange                = 0;
        bool topologyChangeByTimeSliceTimer = false;
        std::vector<std::vector<int>> costMatrix;

        std::vector<routing::Base_Routing*> satRoutingBases;
        std::vector<routing::Ter_Base_Routing *>devicesRoutingBases;
        std::vector<routing::Ter_Base_Routing *>gsRoutingBases;

    private:


        void updateIntraSatelliteLinks();
        void updateInterSatelliteLinks();
        void updateGroundstationLinks();
        void updateLoraDevices();
        void updateISLInWalkerDelta();
        void updateISLInWalkerStar();
};

}


#endif /* GLOBAL_TOPOLOGY_CONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_ */

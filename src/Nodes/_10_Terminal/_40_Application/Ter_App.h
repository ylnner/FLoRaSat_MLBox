/*
 * Ter_App.h
 *
 *  Created on: Oct 22, 2025
 *      Author: root
 */

#ifndef NODES__10_TERMINAL__40_APPLICATION_TER_APP_H_
#define NODES__10_TERMINAL__40_APPLICATION_TER_APP_H_

#include <omnetpp.h>
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
//#include "LoRa/LoRaMacControlInfo_m.h"
//#include "topologycontrol/TopologyControlBase.h"

#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/queueing/base/ActivePacketSinkBase.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Global/Statistics/TransmissionStatistics/TransmissionStatistics.h"
#include "Global/Statistics/TransmissionStatistics/CstTransmissionStatisticsTag_m.h"
#include "Nodes/_10_Terminal/Ter.h"
//#include "Nodes/_10_Terminal/_40_Application/MLBox/Ter_App_MLBox.h"
//#include "Global/Transmission_Predictor/Transmission_Predictor.h"
#include "Global/Messages/_40_Application/LoRaAppPacket_m.h"
#include "Global/Utilities/Utils.h"

using namespace omnetpp;
using namespace inet;
using namespace routing;
using namespace terminal;
using namespace statistics;

class Ter_App : public cSimpleModule, public ILifecycle {
    private:
        std::set<int> satellites;
        //int attempts;
        int loraNodeId;
        int maxHops;

    protected:
        void initialize(int stage) override;
        void finish() override;
        int numInitStages() const override { return NUM_INIT_STAGES; }
        void handleMessage(cMessage *msg) override;
        virtual bool handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback) override;

        //void handleMessageFromLowerLayer(cMessage *msg);
        void sendUplinkPacket();


        void encapsulate(Packet *packet);
        void decapsulate(Packet *packet);
        //void handleTransportInPacket(inet::Packet *pkt);

        bool receivedAck = true;

        int numberOfPacketsToSend;
        int sentPackets;
        int receivedAckPackets;
        int receivedADRCommands;
        int lastSentMeasurement;
        simtime_t timeToFirstPacket;
        simtime_t timeToNextPacket;
        simtime_t timeToResponse;
        simtime_t timeToRetry;

        simtime_t ackTimeout;
        simtime_t timer;

        cMessage *configureLoRaParameters;
        cMessage *sendUplink;
        cMessage *sendResponse;
        cMessage *sendRetry;

        cMessage *endAckTime;
        cMessage *synchronizer;
        cMessage *joining;
        cMessage *joiningAns;

        //history of sent packets;
        cOutVector sfVector;
        cOutVector tpVector;


        //variables to control ADR
        bool evaluateADRinNode;
        int ADR_ACK_CNT                  = 0;
        int ADR_ACK_LIMIT                = 2; //64;
        int ADR_ACK_DELAY                = 1; //32;
        bool sendNextPacketWithADRACKReq = false;
        void increaseSFIfPossible();


        int receivedPackets;
        Ter *ter = nullptr;
        Ter_Mob *ter_mob = nullptr;
        Sat_Mob_NoradA *sat_mob = nullptr;

    public:
        Ter_App() {}
        virtual ~Ter_App();
        simsignal_t LoRa_AppPacketSent;

        std::pair<std::pair<int, int>, std::pair<int, int>> recalculateRoute();

        //LoRa physical layer parameters
        double loRaTP;
        units::values::Hz loRaCF;
        int loRaSF;
        units::values::Hz loRaBW;
        int loRaCR;
        bool loRaUseHeader;
        int pingSlot;
        bool usingAck = false;

    //protected:
      //  Transmission_Predictor *mlbox;
};


#endif /* NODES__10_TERMINAL__40_APPLICATION_TER_APP_H_ */

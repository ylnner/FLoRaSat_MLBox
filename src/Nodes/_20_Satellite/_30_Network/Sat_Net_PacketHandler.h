/*
 * Sat_Net_PacketHandler.h
 *
 *  Created on: Nov 10, 2025
 *      Author: FLoRaSat Team
 */

#ifndef NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETHANDLER_H_
#define NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETHANDLER_H_

#include <omnetpp.h>
#include <omnetpp/cchannel.h>
#include <omnetpp/cpacketqueue.h>

#include <string>
#include <unordered_map>

#include "Global/Utilities/ISLDirection_m.h"
#include "Nodes/_20_Satellite/_30_Network/Sat_Net_PacketSender.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/queueing/base/PacketProcessorBase.h"
#include "Global/Statistics/TransmissionStatistics/TransmissionStatistics.h"
#include "Nodes/_10_Terminal/Ter.h"

#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility_Standalone.h"
#include "Global/Messages/_20_Data_Link/LoRaTagInfo_m.h"
using namespace omnetpp;
using namespace inet;
using namespace core::isldirection;

namespace routing {
class Base_Routing;
}

namespace satellite {

/**
 * @brief Central packet dispatcher implementing the Sat_Net_PacketSender interface
 * 
 * Hub for all network layer packet traffic. Receives packets from all input gates (ISL, GSL, DSL),
 * delegates routing decisions to the routing module, and executes those decisions by forwarding
 * packets to the appropriate output gates.
 * 
 * Key Features:
 * - Per-gate transmission queuing: Each output gate has FIFO queue + metadata storage
 * - Channel conflict resolution: Packets queued when channel busy (based on getTransmissionFinishTime())
 * - Dequeue scheduling: Self-messages trigger transmission when channel becomes free
 * - ISLDirection mapping: Translates abstract directions (LEFT/RIGHT/UP/DOWN/GROUNDLINK) to physical gates
 * 
 * Packet Flow:
 * 1. handleMessage(msg): Receive from input gate → emit packetReceivedSignal
 * 2. routingModule->handlePacket(pkt): Delegate to routing algorithm
 * 3. Routing calls back sender->sendMessage(pkt, direction, silent, gsIndex)
 * 4. Map direction to gate, check connectivity, queue if busy or send immediately
 * 5. Emit packetSentSignal on successful transmission
 * 
 * TransmissionState (per gate):
 * - queue: cPacketQueue for packets waiting on busy channel
 * - dequeueEvent: Self-message scheduled at channel completion time
 * - metadata: Maps Packet* to {ISLDirection, silent, dstGroundIndex} for queued packets
 * 
 * Usage by routing modules:
 * - sender->sendMessage(pkt, ISLDirection, silent, gsIndex): Forward packet
 * - sender->dropPacket(pkt, reason, silent): Drop with statistics
 * - gsIndex from Base_Routing::getGroundlinkIndex(satId, gsId) when direction==GROUNDLINK
 */
class Sat_Net_PacketHandler : public cSimpleModule, public Sat_Net_PacketSender {
  protected:
    int numGroundLinks = 0;
    int satIndex = 0;

    // Reference to routing module
    routing::Base_Routing *routingModule = nullptr;

    // Statistics
    long numPacketsReceived = 0;
    long numPacketsSent = 0;
    long numPacketsDropped = 0;

    cOutVector packetsReceivedVector;
    cOutVector packetsForwardedVector;
    cOutVector packetsDroppedVector;

    struct TransmissionMetadata {
        ISLDirection direction;
        bool silent;
        int dstGroundIndex;
    };

    struct TransmissionState {
        cGate *gate = nullptr;
        omnetpp::cPacketQueue queue;
        cMessage *dequeueEvent = nullptr;
        std::unordered_map<Packet *, TransmissionMetadata> metadata;

        TransmissionState() : queue("txQueue") {}
        TransmissionState(const TransmissionState &) = delete;
        TransmissionState &operator=(const TransmissionState &) = delete;
    };

    std::unordered_map<int, TransmissionState> transmissionStates;

    TransmissionState &ensureTransmissionState(cGate *gate);
    bool isGateBusy(const cGate *gate) const;
    cChannel *resolveTransmissionChannel(const cGate *gate) const;
    simtime_t getChannelReadyTime(const cGate *gate) const;
    void scheduleDequeue(TransmissionState &state);
    void trySendFromQueue(TransmissionState &state);
    void sendOnGate(Packet *pkt, const TransmissionMetadata &meta, TransmissionState &state);
    void handleSelfMessage(cMessage *msg);

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual ~Sat_Net_PacketHandler() override;
    
    /**
     * Get the output gate for a given direction and optional ground station index.
     */
    cGate *getOutputGate(ISLDirection direction, int gsIndex = -1);

    /*
     * Reference to TransmissionStatistics
     * */
    statistics::TransmissionStatistics *transmissionStatistics = nullptr;
  public:
    /**
     * Send a packet in the specified direction.
     * @param pkt The packet to send
  * @param dir The ISL direction (LEFT, RIGHT, UP, DOWN, or GROUNDLINK)
  * @param silent If true, don't emit signals
  * @param dstGs Ground link gate index (as provided by Base_Routing::getGroundlinkIndex, only used when dir == GROUNDLINK)
     */
    virtual void sendMessage(Packet *pkt, ISLDirection dir, bool silent = false, int dstGs = -1) override;
    
    /**
     * Drop a packet with a given reason.
     * @param dropPkt The packet to drop
     * @param reason The reason for dropping
     * @param silent If true, don't emit signals
     */
    virtual void dropPacket(Packet *dropPkt, PacketDropReason reason, bool silent = false) override;
};

}  // namespace satellite

#endif /* NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETHANDLER_H_ */

/*
 * Sat_Net_SatelliteRouting.h
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_SAT_NET_SATELLITEROUTING_H_
#define __FLORA_SATELLITE_SAT_NET_SATELLITEROUTING_H_

#include <omnetpp.h>

#include "Nodes/_20_Satellite/Sat.h"
#include "Global/Utilities/FloraSignals.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "Global/Statistics/PacketRecorder.h"
#include "ForwardingTable.h"
//#include "GroundForwardingTable.h"

using namespace omnetpp;
using namespace inet;

namespace satellite {

/**
 * @brief Statistics collector for satellite network layer (attached via @class annotation)
 * 
 * Responsibilities:
 * - Subscribe to signals from submodules (PacketHandler, routing modules)
 * - Aggregate network layer statistics (data/control packets, bytes, drops by reason)
 * - Record final statistics in finish() for post-simulation analysis
 * - Optionally log to PacketRecorder for detailed packet tracking
 * 
 * Important: handleMessage() throws error if called - all packets are routed through
 * NED connections to packetHandler submodule, not through this compound module.
 * 
 * Signals monitored:
 * - packetSentSignal, packetReceivedSignal (data packets)
 * - controlPacketSentSignal, controlPacketReceivedSignal
 * - packetDroppedSignal (with drop reason details)
 * - packetPushedSignal, packetPulledSignal (queue state tracking)
 * 
 * Statistics recorded:
 * - numPacketSent, bytesPacketSent, numPacketReceived
 * - numControlPacketSent, bytesControlPacketSent, numControlPacketReceived
 * - Drop counts by reason: maxHop, fullQueue, ifDown, noRoute
 */
class Sat_Net_SatelliteRouting : public Sat, cListener {
   public:
    /**
     * @brief Receive and process signals from packet handler and routing modules
     */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

   protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void finish() override;
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

   private:
    /**
     * @brief Handle packet drop events and record statistics
     */
    void handlePacketDropped(cComponent *source, inet::Packet *pkt, inet::PacketDropDetails *reason);

   public:
    void printRoutingTables();

   private:
    // Tables to destination (Ground and Sat)
    //ForwardingTable *satRoutingTable;
    //GroundForwardingTable *groundRoutingTable;

    // Cached pointer to statistics recorder
    statistics::PacketRecorder *packetRecorder;

    // Queue state
    uint16_t queueSize = 0;

    // Data packet statistics
    long numPacketSent;
    B bytesPacketSent;
    long numPacketReceived;

    // Control packet statistics
    long numControlPacketSent;
    B bytesControlPacketSent;
    long numControlPacketReceived;

    // Drop statistics by reason
    long numDroppedMaxHop;
    long numDroppedFullQueue;
    long numDroppedIfDown;
    long numDroppedNoRoute;

    // Statistics vectors for time-series data
    cOutVector droppedMaxHopCountStats;
    cOutVector droppedFullQueueCountStats;
    cOutVector droppedIfDownCountStats;
    cOutVector droppedNoRouteCountStats;
};

}  // namespace satellite

#endif  // __FLORA_SATELLITE_SAT_NET_SATELLITEROUTING_H_

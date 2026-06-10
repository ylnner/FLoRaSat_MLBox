/*
 * PacketRecorder.h
 *
 * Created on: Aug 07, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_STATISTICS_PACKETRECORDER_H_
#define __FLORA_STATISTICS_PACKETRECORDER_H_

#include <omnetpp.h>
#include <fstream>
#include <string>

#include "inet/common/INETUtils.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Packet.h"
#include "Global/Utilities/Utils.h"
#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"

using namespace omnetpp;
using namespace inet;

namespace statistics {

class PacketRecorder : public cSimpleModule {
   public:
    PacketRecorder();

    void recordPacket(inet::Packet *packet, PacketDropReason reason = PacketDropReason::OTHER_PACKET_DROP);
    void recordQueueSize(int satId, u_int16_t queueSize);

   protected:
    virtual ~PacketRecorder();

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

   protected:
    int numPackets = 0;
    std::ofstream fileQueue;
    std::ofstream fileStats;
    std::ofstream fileRoutes;
};

}  // namespace statistics

#endif  // __FLORA_STATISTICS_PACKETRECORDER_H_

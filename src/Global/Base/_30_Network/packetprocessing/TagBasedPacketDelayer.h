/*
 * TagBasedPacketDelayer.h
 *
 *  Created on: Feb 10, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_BASE__30_NETWORK_PACKETPROCESSING_TAGBASEDPACKETDELAYER_H_
#define GLOBAL_BASE__30_NETWORK_PACKETPROCESSING_TAGBASEDPACKETDELAYER_H_

#include <omnetpp.h>

#include "DelayPacketTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/queueing/common/PacketDelayer.h"
#include "Global/Messages/_30_Network/Net_QueueInsertionTimeTag_m.h"

using namespace inet::queueing;
using namespace inet;
using namespace omnetpp;

namespace packetprocessing {

class TagBasedPacketDelayer : public inet::queueing::PacketDelayer {
   protected:
    virtual void handleMessage(cMessage *message) override;

   public:
    virtual void pushPacket(Packet *packet, cGate *gate) override;
};

}  // namespace packetprocessing




#endif /* GLOBAL_BASE__30_NETWORK_PACKETPROCESSING_TAGBASEDPACKETDELAYER_H_ */

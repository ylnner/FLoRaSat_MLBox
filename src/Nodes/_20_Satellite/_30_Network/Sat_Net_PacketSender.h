/*
 * Sat_Net_PacketSender.h
 *
 *  Created on: Oct 20, 2025
 *      Author: root
 */

#ifndef NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETSENDER_H_
#define NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETSENDER_H_


#include "Global/Utilities/ISLDirection_m.h"
#include "inet/common/Simsignals_m.h"
#include "inet/common/packet/Packet.h"

using namespace core::isldirection;
using namespace inet;

namespace satellite {

/**
 * @brief Interface for routing modules to send/drop packets
 * 
 * Implemented by Sat_Net_PacketHandler. Routing algorithms receive a pointer
 * to this interface (Base_Routing::sender) and use it to execute routing decisions.
 * 
 * Methods:
 * - sendMessage(): Forward packet in specified direction
 *   - dir: ISLDirection enum (LEFT, RIGHT, UP, DOWN, GROUNDLINK)
 *   - dstGs: Ground link gate index (only used when dir==GROUNDLINK)
 *            Obtain via Base_Routing::getGroundlinkIndex(satId, gsId)
 *   - silent: If true, suppress signal emission (for internal operations)
 * 
 * - dropPacket(): Drop packet with reason for statistics
 *   - reason: PacketDropReason (HOP_LIMIT_REACHED, QUEUE_OVERFLOW, INTERFACE_DOWN, NO_ROUTE_FOUND)
 *   - silent: If true, suppress signal emission
 * 
 * Ownership: Routing module owns packet when handlePacket() called, must either
 * send via sendMessage() or drop via dropPacket(). Both methods take ownership.
 */
class Sat_Net_PacketSender {
   public:
    virtual void sendMessage(Packet *sendPkt, ISLDirection dir, bool silent, int dstGs = -1) = 0;

    virtual void dropPacket(Packet *dropPkt, PacketDropReason reason, bool silent) = 0;
};

}  // namespace satellite




#endif /* NODES__20_SATELLITE__30_NETWORK_SAT_NET_PACKETSENDER_H_ */

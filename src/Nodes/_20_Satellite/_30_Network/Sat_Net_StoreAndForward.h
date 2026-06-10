#ifndef STORE_AND_FORWARD_H
#define STORE_AND_FORWARD_H

#include <vector>
#include <string>
#include "inet/common/packet/Packet.h"
#include "Global/Base/_30_Network/Base_Routing.h"
#include "omnetpp.h"
#include "Sat_Net_SdrModel.h"

using namespace omnetpp;

namespace routing {

/**
 * @brief Store-and-Forward routing for LEO satellite data relay
 * 
 * Simple opportunistic forwarding: Store packets when isolated, forward when ground station visible.
 * 
 * Algorithm:
 * 1. handlePacket(pkt):
 *    - If connected to any GS: Find first available ground link, forward via sender->sendMessage()
 *    - If not connected: Store in sdrModel buffer (drop if full)
 * 
 * 2. handleTopologyChange(bool changes):
 *    - Query all GS connections via getGroundlinkIndex(satId, gsId)
 *    - If newly connected: Trigger checkAndForwardStoredPackets()
 *    - If disconnected: Cancel pending forwarding timers
 * 
 * 3. Stored packet forwarding (paced to avoid overwhelming PacketHandler):
 *    - forwardingTimer scheduled every 1ms
 *    - Each timer: Dequeue one packet from sdrModel, call sender->sendMessage()
 *    - Continue until queue empty or connection lost
 * 
 * Storage: Sat_Net_SdrModel (FIFO queue, configurable max size)
 * 
 * Parameters (in .ned):
 * - maxStoredPackets: SDR buffer capacity (default: 1000)
 * 
 */
class Sat_Net_StoreAndForward : public Base_Routing {
public:
    Sat_Net_StoreAndForward();
    virtual ~Sat_Net_StoreAndForward();
    
    /**
     * @brief Handle incoming packet - decide whether to forward or store
     * @param pkt The packet to handle
     */
    virtual void handlePacket(inet::Packet* pkt) override;
    
    /**
     * @brief Handle topology changes (ISL/GSL connections)
     * @param topologyChanges True if actual topology changes occurred
     */
    virtual void handleTopologyChange(bool topologyChanges) override;

    
    /**
     * @brief Update ground station connection status
     * @param connected True if connected to any ground station
     */
    void setGroundStationConnected(bool connected);
    
    /**
     * @brief Check for stored packets and forward them if GS is available
     */
    void checkAndForwardStoredPackets();
    
    /**
     * @brief Handle self-messages (forwarding timer)
     */
    virtual void handleMessage(cMessage* msg) override;

    

protected:
    Sat_Net_SdrModel* sdrModel;                 // Storage model for packets
    bool isConnectedToGroundStation;    // Connection status
    cMessage* forwardingTimer;          // Timer for pacing packet forwarding
    
    /**
     * @brief Forward the next stored packet if conditions allow
     */
    void tryForwardNextStoredPacket();
};
}

#endif

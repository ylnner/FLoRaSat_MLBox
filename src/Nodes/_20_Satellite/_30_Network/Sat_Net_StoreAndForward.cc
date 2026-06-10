#include "Sat_Net_StoreAndForward.h"

namespace routing {

Define_Module(Sat_Net_StoreAndForward);

Sat_Net_StoreAndForward::Sat_Net_StoreAndForward() {
    sdrModel = new Sat_Net_SdrModel();
    isConnectedToGroundStation = false;
    forwardingTimer = nullptr;
    EV_DEBUG << "Sat_Net_StoreAndForward created" << endl;
}

Sat_Net_StoreAndForward::~Sat_Net_StoreAndForward() {
    delete sdrModel;
    if (forwardingTimer) {
        cancelAndDelete(forwardingTimer);
        forwardingTimer = nullptr;
    }
    EV_DEBUG << "Sat_Net_StoreAndForward destroyed" << endl;
}

void Sat_Net_StoreAndForward::handlePacket(inet::Packet* pkt) {
    Enter_Method("handlePacket");
    
    EV_DEBUG << "Handling packet: ID=" << pkt->getId() << endl;
    
    // Check if connected to any ground station
    if (isConnectedToGroundStation) {
        // Connected to a ground station, forward the packet immediately
    EV_DEBUG << "Connected to a ground station, forwarding immediately regardless of destination" << endl;
        
        // Find the first available ground station
        int connectedGsId = -1;
        int connectedGateIndex = -1;
        int numGs = topologyControl->getNumberOfGroundstations();
        for (int gsId = 0; gsId < numGs; gsId++) {
            int gateIndex = getGroundlinkIndex(satId, gsId, 0);
            if (gateIndex != -1) {
                connectedGsId = gsId;
                connectedGateIndex = gateIndex;
                break;
            }
        }
        
        if (connectedGateIndex == -1) {
            EV_WARN << "Ground station connectivity flag was true but no ground link gate is available, dropping packet ID=" << pkt->getId() << endl;
            sender->dropPacket(pkt, PacketDropReason::INTERFACE_DOWN, false);
            return;
        }

    EV_DEBUG << "Forwarding via ground link gate " << connectedGateIndex
        << " (GS " << connectedGsId << ")" << endl;

    // Forward through the packet handler
    sender->sendMessage(pkt, ISLDirection::GROUNDLINK, false, connectedGateIndex);
        return;
    }
    
    // Not connected to any ground station, store for later
    EV_DEBUG << "Not connected to any ground station, storing packet" << endl;
    bool stored = sdrModel->enqueuePacket(pkt);
    if (!stored) {
        // If storage failed (queue full), drop the packet
        EV_WARN << "Storage queue full, dropping packet ID=" << pkt->getId() << endl;
        sender->dropPacket(pkt, PacketDropReason::QUEUE_OVERFLOW, true);
    }
}

void Sat_Net_StoreAndForward::handleMessage(cMessage* msg) {
    if (msg == forwardingTimer) {
        // Time to forward the next stored packet
        tryForwardNextStoredPacket();
    } else {
        // Let the base class handle other messages
        Base_Routing::handleMessage(msg);
    }
}

void Sat_Net_StoreAndForward::setGroundStationConnected(bool connected) {
    // Check if this is a new connection to a ground station
    bool newConnection = !isConnectedToGroundStation && connected;
    
    // EV_INFO << "Ground station connection state change: " << (isConnectedToGroundStation ? "connected" : "disconnected") 
    //        << " -> " << (connected ? "connected" : "disconnected") << endl;
    
    // Update connection status
    isConnectedToGroundStation = connected;
    
    // If this is a new connection, initiate stored packet forwarding
    if (newConnection) {
        EV_DEBUG << "New ground station connection detected, initiating stored packet forwarding" << endl;
        checkAndForwardStoredPackets();
    }
    
    // If we lost the connection, cancel any pending forwarding timer
    if (!connected && forwardingTimer && forwardingTimer->isScheduled()) {
        EV_DEBUG << "Lost ground station connection, canceling pending forwarding timer" << endl;
        cancelEvent(forwardingTimer);
    }
}

void Sat_Net_StoreAndForward::checkAndForwardStoredPackets() {
    // Only proceed if we have a ground station connection
    if (!isConnectedToGroundStation) {
        // EV_INFO << "Not connected to any ground station, skipping stored packet forwarding" << endl;
        return;
    }
    
    // Check if we have stored packets
    if (!sdrModel->hasPackets()) {
        // EV_INFO << "No stored packets to forward" << endl;
        return;
    }
    
    // Find the first available ground station
    int connectedGsId = -1;
    int connectedGateIndex = -1;
    int numGs = topologyControl->getNumberOfGroundstations();
    for (int gsId = 0; gsId < numGs; gsId++) {
        int gateIndex = getGroundlinkIndex(satId, gsId, 0);
        if (gateIndex != -1) {
            connectedGsId = gsId;
            connectedGateIndex = gateIndex;
            break;
        }
    }
    
    if (connectedGsId == -1 || connectedGateIndex == -1) {
        EV_WARN << "Ground station connection flag is true but no actual connection found" << endl;
        return;
    }
    
    EV_DEBUG << "Beginning forwarding of stored packets to ground station " << connectedGsId
        << " through gate " << connectedGateIndex << endl;
    
    // Initialize the forwarding timer if not already created
    if (!forwardingTimer) {
        forwardingTimer = new cMessage("forwardingTimer");
    }
    
    // Start forwarding the first packet immediately
    tryForwardNextStoredPacket();
}

void Sat_Net_StoreAndForward::tryForwardNextStoredPacket() {
    // Only proceed if we have a ground station connection
    if (!isConnectedToGroundStation) {
        EV_DEBUG << "Lost ground station connection, stopping stored packet forwarding" << endl;
        return;
    }
    
    // Check if we have more packets to forward
    if (!sdrModel->hasPackets()) {
        EV_DEBUG << "No more stored packets to forward" << endl;
        return;
    }
    
    // Find the connected ground station
    int connectedGsId = -1;
    int connectedGateIndex = -1;
    int numGs = topologyControl->getNumberOfGroundstations();
    for (int gsId = 0; gsId < numGs; gsId++) {
        int gateIndex = getGroundlinkIndex(satId, gsId, 0);
        if (gateIndex != -1) {
            connectedGsId = gsId;
            connectedGateIndex = gateIndex;
            break;
        }
    }
    
    if (connectedGsId == -1 || connectedGateIndex == -1) {
        EV_WARN << "Ground station connection lost during forwarding, stopping" << endl;
        return;
    }
    
    // Dequeue and forward one packet
    inet::Packet* pkt = sdrModel->dequeueNextPacket();
    if (pkt != nullptr) {
        EV_DEBUG << "Packet dequeued: ID=" << pkt->getId() 
                << ", Remaining queue size=" << sdrModel->getQueueSize() << endl;
        EV_DEBUG << "Processing stored packet: ID=" << pkt->getId()
                << ", forwarding to available GS=" << connectedGsId << endl;
        
        // Send the packet through the packet handler (which has its own queuing)
        sender->sendMessage(pkt, ISLDirection::GROUNDLINK, false, connectedGateIndex);
        
        // Schedule the next forwarding attempt
        // The packet handler will queue it properly, but we should space out our forwarding
        // requests to avoid overwhelming the system. Use a small delay.
        if (sdrModel->hasPackets()) {
            // Schedule the next forwarding attempt after a small delay (e.g., 1ms)
            // This allows the packet handler's queue to process packets properly
            simtime_t nextForwardTime = simTime() + SimTime(1, SIMTIME_MS);
            scheduleAt(nextForwardTime, forwardingTimer);
        } else {
            EV_DEBUG << "All stored packets have been forwarded" << endl;
        }
    }
}

void Sat_Net_StoreAndForward::handleTopologyChange(bool topologyChanges) {
    Enter_Method("handleTopologyChange");
    
    // EV_INFO << "Handling topology change, topologyChanges=" << (topologyChanges ? "true" : "false") << endl;
    
    // Check for ground station connectivity
    bool connectedToAnyGS = false;
    int numGs = topologyControl->getNumberOfGroundstations();
    
    for (int gsId = 0; gsId < numGs; gsId++) {
        if (getGroundlinkIndex(satId, gsId) != -1) {
            // EV_INFO << "Connected to ground station GS=" << gsId << endl;
            connectedToAnyGS = true;
            break;
        }
    }
    
    if (!connectedToAnyGS) {
        EV_DEBUG << "Not connected to any ground station" << endl;
    }
    
    // Update connection status and forward packets if needed
    setGroundStationConnected(connectedToAnyGS);
}

}

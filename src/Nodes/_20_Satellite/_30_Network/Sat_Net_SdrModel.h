#ifndef SAT_NET_SDR_MODEL_H
#define SAT_NET_SDR_MODEL_H

#include <deque>
#include "inet/common/packet/Packet.h"

/**
 * @brief Software Defined Radio storage model - FIFO packet buffer for store-and-forward
 * 
 * Simple packet storage abstraction used by Sat_Net_StoreAndForward to buffer packets
 * when satellite is not connected to ground stations.
 * 
 * Implementation:
 * - std::deque<inet::Packet*>: FIFO queue (enqueue at back, dequeue from front)
 * - maxQueueSize: Configurable capacity limit (default: 10000)
 * 
 * Memory Management:
 * - Takes ownership of stored packets via enqueuePacket()
 * - Returns ownership via dequeueNextPacket()
 * - Destructor cleans up any remaining packets
 * 
 * Methods:
 * - enqueuePacket(): Add to buffer, returns false if full (caller must drop packet)
 * - dequeueNextPacket(): Remove and return oldest packet (nullptr if empty)
 * - hasPackets(): Quick check for non-empty queue
 * - getQueueSize(): Current buffer occupancy
 * - setMaxQueueSize(): Adjust capacity limit
 * 
 * Usage in StoreAndForward:
 * 1. Packet arrives when GS not visible → sdrModel->enqueuePacket(pkt)
 * 2. GS becomes visible → repeatedly call dequeueNextPacket() with pacing
 * 3. Forward dequeued packets via sender->sendMessage()
 */
class Sat_Net_SdrModel {
public:
    Sat_Net_SdrModel();
    virtual ~Sat_Net_SdrModel();
    
    // Enqueue a packet for storage (returns false if queue full)
    bool enqueuePacket(inet::Packet* packet);
    
    // Dequeue the next packet for transmission (FIFO order, nullptr if empty)
    inet::Packet* dequeueNextPacket();
    
    // Check if there are packets in the queue
    bool hasPackets() const { return !packetQueue.empty(); }
    
    // Get the current queue size
    int getQueueSize() const { return packetQueue.size(); }
    
    // Set the maximum queue size
    void setMaxQueueSize(int size) { maxQueueSize = size; }

private:
    std::deque<inet::Packet*> packetQueue;
    int maxQueueSize;
};

#endif // SAT_NET_SDR_MODEL_H

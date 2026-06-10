#include "Sat_Net_SdrModel.h"

using namespace inet;

Sat_Net_SdrModel::Sat_Net_SdrModel() {
    maxQueueSize = 10000; // default value
    // EV_INFO << "SDR Model initialized with max queue size: " << maxQueueSize << endl;
}

Sat_Net_SdrModel::~Sat_Net_SdrModel() {
    // EV_INFO << "SDR Model destroying " << packetQueue.size() << " remaining packets" << endl;
    // Clean up any remaining packets
    for (auto packet : packetQueue) {
        delete packet;
    }
    packetQueue.clear();
}

bool Sat_Net_SdrModel::enqueuePacket(inet::Packet* packet) {
    if ((int)packetQueue.size() >= maxQueueSize) {
        EV_WARN << "DROPPED PACKET: Queue full (size=" << packetQueue.size() << ")" << endl;
        return false;
    }
    
    // Add to the back of the queue
    packetQueue.push_back(packet);
    EV_INFO << "Packet enqueued: ID=" << packet->getId() 
            << ", Queue size=" << packetQueue.size() 
            << "/" << maxQueueSize << endl;
    return true;
}

inet::Packet* Sat_Net_SdrModel::dequeueNextPacket() {
    if (packetQueue.empty()) {
        EV_INFO << "Dequeue attempt on empty queue" << endl;
        return nullptr;
    }
    
    // Get the first packet (front of queue)
    inet::Packet* packet = packetQueue.front();
    packetQueue.pop_front();
    EV_INFO << "Packet dequeued: ID=" << packet->getId() 
            << ", Remaining queue size=" << packetQueue.size() << endl;
    return packet;
}

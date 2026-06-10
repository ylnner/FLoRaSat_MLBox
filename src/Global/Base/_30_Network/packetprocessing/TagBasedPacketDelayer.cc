/*
 * TagBasedPacketDelayer.cc
 *
 *  Created on: Feb 10, 2026
 *      Author: ylnner
 */




#include "TagBasedPacketDelayer.h"

namespace packetprocessing {

Define_Module(TagBasedPacketDelayer);

void TagBasedPacketDelayer::handleMessage(cMessage *message) {
    if (message->isSelfMessage()) {
        auto packet = check_and_cast<Packet *>(message);
        auto tag = packet->addTagIfAbsent<satellite::Sat_Net_QueueInsertionTimeTag>();
        tag->setInsertionTime(simTime());
        pushOrSendPacket(packet, outputGate, consumer);
    } else
        PacketPusherBase::handleMessage(message);
}

void TagBasedPacketDelayer::pushPacket(Packet *packet, cGate *gate) {
    Enter_Method("pushPacket");
    take(packet);

    auto tag = packet->removeTag<DelayPacketTag>();
    simtime_t delay = tag->getDelay();
    EV_DEBUG << "Delaying packet " << packet->getId() << " for " << delay << endl;
    scheduleAfter(delay, packet);
    handlePacketProcessed(packet);
    updateDisplayString();
}

}  // namespace packetprocessing

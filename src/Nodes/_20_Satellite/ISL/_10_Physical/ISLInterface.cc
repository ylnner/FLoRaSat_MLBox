/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "ISLInterface.h"

#include "inet/common/packet/printer/PacketPrinter.h"
#include "Global/Messages/_30_Network/Net_QueueInsertionTimeTag_m.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"

using namespace satellite;
using namespace inet;
using namespace inet::queueing;


namespace satellite {

Define_Module(ISLInterface);

void ISLInterface::initialize(int stage) {
    ClockUserModuleMixin::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        collectionTimer = new ClockEvent("CollectionTimer");
        islOutGate = gate("islOut");
        ///////////sender = check_and_cast<PacketHandlerRouting *>(getParentModule()->getParentModule()->getSubmodule("packetHandler"));
    }
}

void ISLInterface::handleMessage(cMessage *msg) {
    // events
    if (msg == collectionTimer) {
        EV_DEBUG << simTime().dbl() << ": Collect packet due to timer." << endl;
        collectPacket();
    }
}

void ISLInterface::collectPacket() {
    cGate *gate = inputGate->getPathStartGate();
    if (!provider->canPullSomePacket(gate)) {
        EV_DEBUG << simTime().dbl() << ": Cannot pull another packet" << endl;
        return;
    }

    cChannel *channel = islOutGate->findTransmissionChannel();
    if (channel == nullptr) {
        Packet *pkt = provider->pullPacket(gate);
        take(pkt);

        EV_DEBUG << simTime().dbl() << ": Not connected, delete pkt" << pkt->getId() << endl;

        // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes; use simtime_t
        simtime_t insertedAt = pkt->removeTag<Sat_Net_QueueInsertionTimeTag>()->getInsertionTime();
        simtime_t queueTime = simTime() - insertedAt;
        pkt->getTagForUpdate<inet::QueueingTimeTag>()->appendBitTotalTimes(queueTime);

        handlePacketProcessed(pkt);
        numDroppedPackets++;
        ///////////sender->dropPacket(pkt, PacketDropReason::INTERFACE_DOWN, true);
        collectPacket();
    } else {
        if (channel->isBusy()) {
            if (!collectionTimer->isScheduled()) {
                EV_DEBUG << simTime().dbl() << ": Channel busy, schedule new event at " << channel->getTransmissionFinishTime().dbl() << endl;
                cancelClockEvent(collectionTimer);
                scheduleClockEventAt(clocktime_t(channel->getTransmissionFinishTime().dbl()), collectionTimer);
            } else {
                EV_DEBUG << simTime().dbl() << ": Channel busy, but event already scheduled for " << collectionTimer->getArrivalTime().dbl() << endl;
            }
        } else {
            Packet *pkt = provider->pullPacket(gate);
            take(pkt);
            EV_DEBUG << simTime().dbl() << ": Send packet " << pkt->getId() << endl;
            // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes; use simtime_t
            simtime_t queueTime = simTime() - pkt->removeTag<Sat_Net_QueueInsertionTimeTag>()->getInsertionTime();
            pkt->getTagForUpdate<inet::QueueingTimeTag>()->appendBitTotalTimes(queueTime);

            handlePacketProcessed(pkt);
            send(pkt, "islOut");
            // transmission delay
            simtime_t transmissionFinishTime = channel->getTransmissionFinishTime();
            simtime_t transmissionTime = transmissionFinishTime - simTime();
            ASSERT(transmissionTime > 0);
            // INET 4.5: TransmissionTimeTag uses appendBitTotalTimes() instead of appendTotalTimes()
            pkt->getTagForUpdate<inet::TransmissionTimeTag>()->appendBitTotalTimes(transmissionTime);
            // init for propagation delay
            pkt->addTagIfAbsent<SendAtTag>()->setSendAt(transmissionFinishTime);
            collectPacket();
        }
    }
    updateDisplayString();
}

void ISLInterface::handleCanPullPacketChanged(cGate *gate) {
    Enter_Method("handleCanPullPacketChanged");
    // if there is currently no packet in "processing", schedule the processing of the next package
    EV_DEBUG << simTime().dbl() << ": Can pull packet" << endl;
    collectPacket();
}

void ISLInterface::handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) {
    Enter_Method("handlePullPacketProcessed");
}

std::string ISLInterface::resolveDirective(char directive) const {
    static std::string result;
    switch (directive) {
        case 'p':
            result = std::to_string(numProcessedPackets);
            break;
        case 'd':
            result = std::to_string(numDroppedPackets);
            break;
        case 'l':
            result = processedTotalLength.str();
            break;
        default:
            throw cRuntimeError("Unknown directive: %c", directive);
            break;
    }
    return result;
}

}  // namespace satellite

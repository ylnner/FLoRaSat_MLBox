/*
 * Sat_Net_PacketHandler.cc
 *
 *  Created on: Nov 10, 2025
 *      Author: FLoRaSat Team
 */

#include "Sat_Net_PacketHandler.h"
#include "Global/Base/_30_Network/Base_Routing.h"
#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"

#include <algorithm>
#include <string>
#include "omnetpp/cchannel.h"

namespace satellite {

Define_Module(Sat_Net_PacketHandler);

namespace {
constexpr const char *kDequeueTimerSuffix = "-dequeue";
}

void Sat_Net_PacketHandler::initialize(int stage) {
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        numGroundLinks = par("numGroundLinks").intValue();

        // Initialize statistics
        numPacketsSent = 0;
        numPacketsDropped = 0;
        numPacketsReceived = 0;

        packetsForwardedVector.setName("packets forwarded");
        packetsDroppedVector.setName("packets dropped");
        packetsReceivedVector.setName("packets received");

        // Prepare routing reference and per-gate state
        routingModule = nullptr;
        transmissionStates.clear();

        // Getting satIndex
        Sat_Net_SatelliteRouting *sat_aux = check_and_cast<Sat_Net_SatelliteRouting *>(getParentModule());
        satIndex = sat_aux->getId();

        cModule* trxStats = getSystemModule()->getSubmodule("transmissionStatistics");
        if(trxStats != nullptr){
            transmissionStatistics = check_and_cast<statistics:: TransmissionStatistics *>(trxStats);
        }

    } else if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        cModule *parent = getParentModule();
        if (parent) {
            std::string routingType = parent->par("routingType").stdstringValue();
            if(routingType == "Null_Routing")
                return;

            routingModule = check_and_cast<routing::Base_Routing *>(parent->getSubmodule("routing"));
        }
        if (!routingModule) {
            throw cRuntimeError("Sat_Net_PacketHandler: Could not find routing module");
        }
    }
}

Sat_Net_PacketHandler::~Sat_Net_PacketHandler() {
    for (auto &entry : transmissionStates) {
        TransmissionState &state = entry.second;
        if (state.dequeueEvent != nullptr) {
            cancelAndDelete(state.dequeueEvent);
            state.dequeueEvent = nullptr;
        }

        while (!state.queue.isEmpty()) {
            Packet *pkt = check_and_cast<Packet *>(state.queue.pop());
            state.metadata.erase(pkt);
            delete pkt;
        }

        state.metadata.clear();
    }

    transmissionStates.clear();
}

void Sat_Net_PacketHandler::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
        return;
    }

    if (!msg->isPacket()) {
        EV_ERROR << "Received non-packet message: " << msg->getName() << endl;
        delete msg;
        return;
    }

    Packet *pkt = check_and_cast<Packet *>(msg);
    numPacketsReceived++;
    packetsReceivedVector.record(numPacketsReceived);

    EV_INFO << "PacketHandler received packet " << pkt->getName()
            << " from gate " << pkt->getArrivalGate()->getName() << endl;

    emit(packetReceivedSignal, pkt);

    Sat_Net_SatelliteRouting *sat = check_and_cast<Sat_Net_SatelliteRouting *>(getParentModule());
    sat->printRoutingTables();

    try{
        auto routingTag = pkt->getTag<routing::CstRoutingTag>();
        auto hops = routingTag->getHops();

        if(hops < routingTag->getMaxHops()){
            if (routingModule) {
                routingModule->handlePacket(pkt);
            } else {
                EV_ERROR << "Routing module not available, dropping packet" << endl;
                dropPacket(pkt, PacketDropReason::NO_ROUTE_FOUND, false);
            }
        }else{
            EV << "ELSE hops < tag->getMaxHops()"<<endl;
            routingModule->handleMaxHopsReached(pkt);
        }
    }catch(const std::exception &NoSuchRouteException){
        EV << "catch PacketHandlerRouting::handleMessage" <<endl;
        //numPacketsDroppedNoRouteFound++;
        dropPacket(pkt, PacketDropReason::NO_ROUTE_FOUND, false);
    }

}

void Sat_Net_PacketHandler::finish() {
    cSimpleModule::finish();

    EV << "Packets received: " << numPacketsReceived << endl;
    EV << "Packets sent: " << numPacketsSent << endl;
    EV << "Packets dropped: " << numPacketsDropped << endl;

    recordScalar("packetsReceived", numPacketsReceived);
    recordScalar("packetsSent", numPacketsSent);
    recordScalar("packetsDropped", numPacketsDropped);
}

cGate *Sat_Net_PacketHandler::getOutputGate(ISLDirection direction, int gsIndex) {
    cGate *outputGate = nullptr;

    switch (direction) {
        case ISLDirection::LEFT:
            outputGate = gate("leftOut");
            break;
        case ISLDirection::RIGHT:
            outputGate = gate("rightOut");
            break;
        case ISLDirection::UP:
            outputGate = gate("upOut");
            break;
        case ISLDirection::DOWN:
            outputGate = gate("downOut");
            break;
        case ISLDirection::GROUNDLINK:{
            if (gsIndex < 0 || gsIndex >= numGroundLinks) {
                EV_ERROR << "Requested ground link gate index " << gsIndex
                         << " out of range (0-" << (numGroundLinks - 1) << ")" << endl;
                return nullptr;
            }

            int gateIndex = routingModule->getGroundlinkIndex(satIndex, gsIndex);
            if (gateIndex == -1){
                return nullptr;
            }
            //outputGate = gate("groundLinkOut", gsIndex);
            outputGate = gate("groundLinkOut", gateIndex);
        } break;
        default:
            EV_ERROR << "Unknown ISL direction: " << static_cast<int>(direction) << endl;
            return nullptr;
    }

    return outputGate;
}

void Sat_Net_PacketHandler::sendMessage(Packet *pkt, ISLDirection dir, bool silent, int dstGs) {
    Enter_Method("sendMessage");
    EV << "Sat_Net_PacketHandler::sendMessage" <<endl;
    cGate *outputGate = getOutputGate(dir, dstGs);
    EV << "outputGate->getName(): "<< outputGate->getName() <<endl;
    if (outputGate == nullptr) {
        take(pkt);
        EV_ERROR << "Cannot send packet: no valid output gate for direction " << static_cast<int>(dir) << endl;
        dropPacket(pkt, PacketDropReason::NO_ROUTE_FOUND, silent);
        return;
    }

    if (!outputGate->isConnected()) {
        take(pkt);
        EV_WARN << "Output gate not connected for direction " << static_cast<int>(dir) << endl;
        dropPacket(pkt, PacketDropReason::INTERFACE_DOWN, silent);
        return;
    }

    take(pkt);

    TransmissionState &state = ensureTransmissionState(outputGate);
    TransmissionMetadata meta{dir, silent, dstGs};

    // Always use the queue mechanism to ensure proper serialization
    // Queue the packet if: channel is busy, queue is not empty, or dequeue event is pending
    const bool queueRequired = isGateBusy(outputGate) || 
                                !state.queue.isEmpty() || 
                                state.dequeueEvent->isScheduled();
    
    if (queueRequired) {
        state.metadata.emplace(pkt, meta);
        state.queue.insert(pkt);
        EV_DEBUG << "Queueing packet " << pkt->getName() << " for gate " << outputGate->getFullName() 
                 << " (queue size: " << state.queue.getLength() << ")" << endl;
        scheduleDequeue(state);
        return;
    }

    // Channel is free and queue is empty - send immediately
    sendOnGate(pkt, meta, state);
}

void Sat_Net_PacketHandler::dropPacket(Packet *dropPkt, PacketDropReason reason, bool silent) {
    Enter_Method("dropPacket");

    if (!silent) {
        PacketDropDetails details;
        details.setReason(reason);
        details.setLimit(-1);
        emit(packetDroppedSignal, dropPkt, &details);
    }

    numPacketsDropped++;
    packetsDroppedVector.record(numPacketsDropped);

    EV_WARN << "Dropping packet " << dropPkt->getName() << " reason: " << static_cast<int>(reason) << endl;

    delete dropPkt;
}

Sat_Net_PacketHandler::TransmissionState &Sat_Net_PacketHandler::ensureTransmissionState(cGate *gate) {
    const int gateId = gate->getId();
    auto [it, inserted] = transmissionStates.try_emplace(gateId);

    if (inserted) {
        TransmissionState &state = it->second;
        state.gate = gate;
        state.queue.setName((std::string(gate->getFullName()) + "-queue").c_str());
        state.queue.setTakeOwnership(true);
        state.dequeueEvent = new cMessage((std::string(gate->getFullName()) + kDequeueTimerSuffix).c_str());
        state.dequeueEvent->setContextPointer(gate);
    }

    return it->second;
}

bool Sat_Net_PacketHandler::isGateBusy(const cGate *gate) const {
    if (gate == nullptr) {
        return false;
    }

    return getChannelReadyTime(gate) > simTime();
}

cChannel *Sat_Net_PacketHandler::resolveTransmissionChannel(const cGate *gate) const {
    if (gate == nullptr) {
        return nullptr;
    }

    if (cChannel *txChannel = gate->getTransmissionChannel()) {
        return txChannel;
    }

    return gate->getChannel();
}

simtime_t Sat_Net_PacketHandler::getChannelReadyTime(const cGate *gate) const {
    if (cChannel *channel = resolveTransmissionChannel(gate)) {
        return channel->getTransmissionFinishTime();
    }

    return simTime();
}

void Sat_Net_PacketHandler::scheduleDequeue(TransmissionState &state) {
    if (state.queue.isEmpty()) {
        if (state.dequeueEvent->isScheduled()) {
            cancelEvent(state.dequeueEvent);
        }
        return;
    }

    simtime_t readyTime = std::max(simTime(), getChannelReadyTime(state.gate));

    if (state.dequeueEvent->isScheduled()) {
        // Only reschedule if the new ready time is later than currently scheduled
        if (state.dequeueEvent->getArrivalTime() <= readyTime) {
            return;
        }
        cancelEvent(state.dequeueEvent);
    }

    scheduleAt(readyTime, state.dequeueEvent);
}

void Sat_Net_PacketHandler::trySendFromQueue(TransmissionState &state) {
    if (state.queue.isEmpty()) {
        return;
    }

    if (isGateBusy(state.gate)) {
        scheduleDequeue(state);
        return;
    }

    Packet *pkt = check_and_cast<Packet *>(state.queue.pop());
    auto metaIt = state.metadata.find(pkt);
    TransmissionMetadata meta{};
    if (metaIt != state.metadata.end()) {
        meta = metaIt->second;
        state.metadata.erase(metaIt);
    } else {
        EV_WARN << "Missing metadata for queued packet " << pkt->getName()
                << " on gate " << state.gate->getFullName() << endl;
        meta.direction = ISLDirection::LEFT;
        meta.silent = false;
        meta.dstGroundIndex = -1;
    }

    sendOnGate(pkt, meta, state);
    
    // Schedule next dequeue only if there are more packets in queue
    // The scheduleDequeue will check the transmission finish time
    if (!state.queue.isEmpty()) {
        scheduleDequeue(state);
    }
}

void Sat_Net_PacketHandler::sendOnGate(Packet *pkt, const TransmissionMetadata &meta, TransmissionState &state) {
    // Double-check that the channel is actually free before sending
    if (isGateBusy(state.gate)) {
        EV_ERROR << "Attempted to send on busy gate " << state.gate->getFullName() 
                 << " - re-queuing packet " << pkt->getName() << endl;
        // Re-queue the packet with its metadata at the front to maintain order
        state.metadata.emplace(pkt, meta);
        if (state.queue.isEmpty()) {
            state.queue.insert(pkt);
        } else {
            state.queue.insertBefore(state.queue.front(), pkt);
        }
        scheduleDequeue(state);
        return;
    }

    if (!meta.silent) {
        emit(packetSentSignal, pkt);
    }

    numPacketsSent++;
    packetsForwardedVector.record(numPacketsSent);

    std::string extraInfo;
    if (meta.direction == ISLDirection::GROUNDLINK && meta.dstGroundIndex >= 0) {
        extraInfo = " (ground link gate " + std::to_string(meta.dstGroundIndex) + ")";
    }

    EV_INFO << "Forwarding packet " << pkt->getName()
            << " to direction " << static_cast<int>(meta.direction) << extraInfo << endl;

    send(pkt, state.gate);
}

void Sat_Net_PacketHandler::handleSelfMessage(cMessage *msg) {
    cGate *gate = static_cast<cGate *>(msg->getContextPointer());
    if (gate == nullptr) {
        EV_WARN << "Dequeue event without gate context" << endl;
        return;
    }

    auto it = transmissionStates.find(gate->getId());
    if (it == transmissionStates.end()) {
        EV_WARN << "Received dequeue event for unknown gate " << gate->getFullName() << endl;
        return;
    }

    ASSERT(msg == it->second.dequeueEvent);
    trySendFromQueue(it->second);
}

}  // namespace satellite


/*
 * Sat_Net_SatelliteRouting.cc
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#include "Sat_Net_SatelliteRouting.h"

namespace satellite {

Define_Module(Sat_Net_SatelliteRouting);

void Sat_Net_SatelliteRouting::initialize(int stage) {
    Sat::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        // Subscribe to packet signals for statistics
        subscribe(packetSentSignal, this);
        subscribe(packetReceivedSignal, this);

        subscribe(controlPacketSentSignal, this);
        subscribe(controlPacketReceivedSignal, this);

        subscribe(packetDroppedSignal, this);

        // Subscribe to queue signals
        subscribe(packetPushedSignal, this);
        subscribe(packetPulledSignal, this);

        // Cached pointers - packetRecorder is optional
        cModule* packetRecorderModule = getSystemModule()->getSubmodule("packetRecorder");
        packetRecorder = packetRecorderModule ? check_and_cast<statistics::PacketRecorder *>(packetRecorderModule) : nullptr;

        // Initialize statistics counters
        numPacketSent = 0;
        bytesPacketSent = B(0);
        numPacketReceived = 0;

        numControlPacketSent = 0;
        bytesControlPacketSent = B(0);
        numControlPacketReceived = 0;

        numDroppedMaxHop = 0;
        numDroppedFullQueue = 0;
        numDroppedIfDown = 0;
        numDroppedNoRoute = 0;

        // Initialize statistics vectors
        droppedMaxHopCountStats.setName("dropped max hops");
        droppedFullQueueCountStats.setName("dropped full queue");
        droppedIfDownCountStats.setName("dropped inf down");
        droppedNoRouteCountStats.setName("dropped unroutable");
        
        queueSize = 0;

        /*
        satRoutingTable = check_and_cast<ForwardingTable *>(getSubmodule("satRoutingTable"));
        groundRoutingTable = check_and_cast<GroundForwardingTable *>(getSubmodule("groundRoutingTable"));

        EV <<"After initialization Routing Tables" <<endl;
        satRoutingTable->printForwardingTable();
        EV <<"=============================================="<<endl;
        groundRoutingTable->printForwardingTable();
        */
    }
}

void Sat_Net_SatelliteRouting::handleMessage(cMessage *msg) {
    // This module is a compound module with @class annotation for statistics collection.
    // All messages should be routed through NED connections to the packetHandler submodule.
    // If we receive a message here, it means there's a missing connection in the NED file.
    /*
    cGate *arrivalGate = msg->getArrivalGate();
    const char *gateName = arrivalGate->getName();
    int gateIndex = arrivalGate->getIndex();
    EV << "gateName: " << arrivalGate->getName() <<endl;
    // Si la puerta es un vector (ej. in[5]), mostramos el índice; si no, solo el nombre.
    if (arrivalGate->isVector()) {
        error("Mensaje recibido en el módulo compuesto por la puerta: %s[%d]. Mensaje: %s",
              gateName, gateIndex, msg->getName());
    } else {
        error("Mensaje recibido en el módulo compuesto por la puerta: %s. Mensaje: %s",
              gateName, msg->getName());
    }
    */
    //getName
    EV << "handleMEssage" <<endl;
    EV << msg->getName() <<endl;
    EV << msg->getArrivalGate()->getFullName() <<endl;
    EV << msg->getSenderModule()->getFullPath()  <<endl;
    ////error("Received message at compound module level. This should not happen - check NED connections. Message: %s", msg->getName());
}

void Sat_Net_SatelliteRouting::printRoutingTables(){
    /*
    EV<<"Sat_Net_SatelliteRuting::printRoutingTables"<<endl;
    satRoutingTable->printForwardingTable();
    EV <<"=============================================="<<endl;
    groundRoutingTable->printForwardingTable();
    */
}

void Sat_Net_SatelliteRouting::finish() {
    Sat::finish();

    // Record final statistics
    recordScalar("packetsSent", numPacketSent);
    recordScalar("bytesSent", bytesPacketSent.get());
    recordScalar("packetsReceived", numPacketReceived);
    
    recordScalar("controlPacketsSent", numControlPacketSent);
    recordScalar("controlBytesSent", bytesControlPacketSent.get());
    recordScalar("controlPacketsReceived", numControlPacketReceived);
    
    recordScalar("droppedMaxHop", numDroppedMaxHop);
    recordScalar("droppedFullQueue", numDroppedFullQueue);
    recordScalar("droppedIfDown", numDroppedIfDown);
    recordScalar("droppedNoRoute", numDroppedNoRoute);

    EV << "=== Satellite Network Statistics ===" << endl;
    EV << "Packets Sent: " << numPacketSent << " | Bytes: " << bytesPacketSent << endl;
    EV << "Packets Received: " << numPacketReceived << endl;
    EV << "Control Packets Sent: " << numControlPacketSent << " | Bytes: " << bytesControlPacketSent << endl;
    EV << "Control Packets Received: " << numControlPacketReceived << endl;
    EV << "Dropped (Full Queue): " << numDroppedFullQueue << endl;
    EV << "Dropped (Max Hops): " << numDroppedMaxHop << endl;
    EV << "Dropped (Interface Down): " << numDroppedIfDown << endl;
    EV << "Dropped (No Route): " << numDroppedNoRoute << endl;
}

void Sat_Net_SatelliteRouting::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) {
    if (signalID == packetDroppedSignal) {
        auto pkt = static_cast<inet::Packet *>(obj);
        auto reason = static_cast<inet::PacketDropDetails *>(details);
        handlePacketDropped(source, pkt, reason);
    } else if (signalID == packetSentSignal) {
        auto pkt = static_cast<inet::Packet *>(obj);
        numPacketSent++;
        bytesPacketSent += pkt->getTotalLength();
    } else if (signalID == packetReceivedSignal) {
        auto pkt = static_cast<inet::Packet *>(obj);
        numPacketReceived++;
    } else if (signalID == controlPacketSentSignal) {
        auto pkt = static_cast<inet::Packet *>(obj);
        numControlPacketSent++;
        bytesControlPacketSent += pkt->getTotalLength();
    } else if (signalID == controlPacketReceivedSignal) {
        auto pkt = static_cast<inet::Packet *>(obj);
        numControlPacketReceived++;
        if (packetRecorder) {
            packetRecorder->recordPacket(pkt);
        }
    } else if (signalID == packetPushedSignal) {
        // record queue size if it was pushed to main satellite queue
        if (source->getParentModule()->getId() == cSimpleModule::getId()) {
            EV_DEBUG << "Handle packet pushed" << endl;
            queueSize++;
            if (packetRecorder) {
                packetRecorder->recordQueueSize(satId, queueSize);
            }
        }
    } else if (signalID == packetPulledSignal) {
        // record packet if it was pulled from main satellite queue
        if (source->getParentModule()->getId() == cSimpleModule::getId()) {
            EV_DEBUG << "Handle packet pulled" << endl;
            queueSize--;
            if (packetRecorder) {
                packetRecorder->recordQueueSize(satId, queueSize);
            }
        }
    } else {
        error("Unhandled received signal.");
    }
}

void Sat_Net_SatelliteRouting::handlePacketDropped(cComponent *source, inet::Packet *pkt, inet::PacketDropDetails *reason) {
    EV_DEBUG << "Dropped: " << pkt << " | Reason: " << reason->getReason() << EV_ENDL;
    if (reason->getReason() == PacketDropReason::HOP_LIMIT_REACHED) {
        numDroppedMaxHop++;
        droppedMaxHopCountStats.record(numDroppedMaxHop);
    } else if (reason->getReason() == PacketDropReason::QUEUE_OVERFLOW) {
        EV_DEBUG << "Handle queue overflow" << endl;
        numDroppedFullQueue++;
        droppedFullQueueCountStats.record(numDroppedFullQueue);
        // record packet if it was from main satellite queue
        if (source->getParentModule()->getId() == cSimpleModule::getId()) {
            queueSize--;
            if (packetRecorder) {
                packetRecorder->recordQueueSize(satId, queueSize);
            }
        }
    } else if (reason->getReason() == PacketDropReason::INTERFACE_DOWN) {
        numDroppedIfDown++;
        droppedIfDownCountStats.record(numDroppedIfDown);
    } else if (reason->getReason() == PacketDropReason::NO_ROUTE_FOUND) {
        numDroppedNoRoute++;
        droppedNoRouteCountStats.record(numDroppedNoRoute);
    } else {
        error("Unhandled drop reason: %d", reason->getReason());
    }
    if (packetRecorder) {
        packetRecorder->recordPacket(pkt, reason->getReason());
    }
}

}  // namespace satellite

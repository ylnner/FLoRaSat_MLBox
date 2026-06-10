/*
 * Ter_Net_TerminalRouting.cc
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */


#include "Ter_Net_TerminalRouting.h"
#include <string>

namespace terminal{

Define_Module(Ter_Net_TerminalRouting);


void Ter_Net_TerminalRouting::initialize(int stage){
    cSimpleModule::initialize(stage);


    if (stage == inet::INITSTAGE_LOCAL) {
        routingModule = nullptr;


    } else if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        cModule *parent = getParentModule();
        if (parent) {
            std::string routingType = par("routingType").stdstringValue();
            /*if (routingType == "Ter_Null_Routing"){
                return;
            }*/
            routingModule = check_and_cast<routing::Ter_Base_Routing *>(getSubmodule("routing"));
        }
        if (!routingModule) {
            throw cRuntimeError("Ter_Net_TerminalRouting: Could not find routing module");
        }
    }
}

void Ter_Net_TerminalRouting::handleMessage(cMessage *msg) {
    // This module is a compound module with @class annotation for statistics collection.
    // All messages should be routed through NED connections to the routing submodule.

    Packet *pkt = check_and_cast<Packet *>(msg);
    if (routingModule) {
        // Add a RoutingHeader to the packet
        routingModule->prepareSendPacket(pkt, -1, -1, -1);
        routingModule->handlePacket(pkt);

        //Send to the next module
        send(pkt, "lowerOut");
    } else {
        EV_ERROR << "Routing module not available, dropping packet" << endl;
        //dropPacket(pkt, PacketDropReason::NO_ROUTE_FOUND, false);
    }
}

}

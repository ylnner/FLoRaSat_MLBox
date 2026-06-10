/*
 * Sat_Net_DDRARouting.cc
 *
 *  Created on: Jan 20, 2026
 *      Author: ylnner
 */


#include "Sat_Net_DDRARouting.h"

//using namespace base::set;
//using namespace core;

using namespace routing::ddra;

namespace routing{

Define_Module(Sat_Net_DDRARouting);

void Sat_Net_DDRARouting::initialize(int stage){
    Base_Routing::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        broadcastId = 0;
        queueThreshold = par("queueThreshold");
    }
}

void Sat_Net_DDRARouting::handleMessage(cMessage *msg) {
    error("DDRARouting::handleMessage(): Unexpected message.");
}

void Sat_Net_DDRARouting::handleTopologyChange(bool topologyChanges) {
    // Enter_Method("handleTopologyChange");
    // reset state
    EV << "DDRARouting::handleTopologyChange"<<endl;
    leftDrops = 0;
    upDrops = 0;
    rightDrops = 0;
    downDrops = 0;

    if (topologyChanges || !failedLinks.empty()) {
        EV << "inside topologyChanges"<<endl;
        costMatrix = ((topologycontrol::ConstellationTopologyControl *)topologyControl)->getCostMatrix();
        failedLinks.clear();
        knownUnroutableSats.clear();
        clearRoutes();

        applyDynamicRoutes();
    }
}

void Sat_Net_DDRARouting::handlePacket(inet::Packet *pkt){
    Enter_Method("handlePacket");
    //EV << "protocol: " << protocol <<endl;

    auto routingTag = pkt->getTagForUpdate<routing::CstRoutingTag>();
    EV << "Sat_Net_DDRARouting::handlePacket" <<endl;
    EV << pkt->getDetailStringRepresentation(evFlags) << endl;

    auto sequence   = dynamicPtrCast<const SequenceChunk>(pkt->peekData());
    int index = 0;
    for (const auto& chunk : sequence->getChunks()) {
        EV << "Chunk #" << index << ": " << chunk->getChunkType()
                << " - length: " << chunk->getChunkLength() << endl;
        chunk->printToStream(EV, cLog::logLevel, 0);
        EV << "\n -"<<endl;
        index = index +1;
    }
    EV << "-------"<<endl;

    auto macFrame = pkt->popAtFront<mac::Base_MacFrame>();


    auto header = pkt->removeAtFront<DDRARoutingHeader>();
    if (header->getType() != PacketType::NORMAL) {
        handleControlPacket(header);
        delete pkt;
        return;
    }

    // Message received in lowerInDSL gate
    if(pkt->arrivedOn("lowerInDSL")){

        if(routingTag->getSrcGsOrDev()==1) { // Src is terminal
            int terId = routingTag->getSrcId();
            EV << "terId: " << terId <<endl;

            Ter_Net_DDRARouting *ter_net_rot = check_and_cast<Ter_Net_DDRARouting*>(getSystemModule()->getSubmodule("terminal", terId)->getSubmodule("net")->getSubmodule("routing"));

            int dstId = ter_net_rot->getClosestDstId(routingTag->getDstGsOrDev());
            EV << "dstId: " << dstId <<endl;
            EV << "satId: "<< satId <<endl;
            int lastSat = -1;
            if(ter_net_rot){
                lastSat = ter_net_rot->getLastSat(dstId, satId);
                EV << "lastSat: " << lastSat <<endl;

                if (lastSat == -1){
                    return;
                }
            }
            //Update CstRoutingTag - Hop
            routingTag->setSrcId(terId);
            routingTag->setDstId(dstId);
            routingTag->setSrcSat(satId);
            routingTag->setDstSat(lastSat);

            //Update DDRARoutingHeader
            header->setSrcGs(terId);
            header->setDstGs(dstId);
            header->setSrcSat(satId);
            header->setDstSat(lastSat);
        }
    }

    pkt->insertAtFront(header);
    EV << "Second insert at front"<<endl;
    pkt->insertAtFront(macFrame);

    int dstSat = header->getDstSat();
    int dstGs = header->getDstGs();
    // check if connected to destination groundstation
    if (getGroundlinkIndex(satId, dstGs, routingTag->getDstGsOrDev()) != -1) {
        EV << "1er sender->sendMessage"<<endl;
        sender->sendMessage(pkt, ISLDirection::GROUNDLINK, false, dstGs);
        return;
    }
    EV << "Before first if" <<endl;
    // check if is non reachable destination
    if (base::set::contains(knownUnroutableSats, dstSat)) {
        EV << "2do sender->sendMessage"<<endl;
        sender->dropPacket(pkt, NO_ROUTE_FOUND, false);
        return;
    }

    // get next hop
    try {
        EV << "sender->sendMessage" <<endl;
        EV << "dstSat: "<< dstSat<<endl;
        //EV << "getNextHop(dstSat): " << getNextHop(dstSat) <<endl;

        sender->sendMessage(pkt, getNextHop(dstSat), false);
    } catch (const routing::core::NoSuchRouteException &nsre) {
        EV << pkt << " \n unroutable!" << endl;
        // hop now known
        EV << "Before Dijkstra"<< endl;
        core::dspa::DijkstraResultEarlyAbort dijkstraRes = core::dspa::runDijkstraEarlyAbort(costMatrix, satId, {dstSat});

        auto path = core::dspa::reconstructPath(dijkstraRes);
        EV << "path.size(): " << path.size()<<endl;
        EV << "Contenido del vector: ";
        for (int v : path) {
            EV << v << " ";
        }
        //EV << "path: " << path <<endl;
        if (path.size() == 0) {
            // no route found
            EV << "Still no route found from " << satId << " to " << dstSat << endl;
            knownUnroutableSats.emplace(dstSat);
            sender->dropPacket(pkt, NO_ROUTE_FOUND, false);
            return;
        };

        int nextSatId = path[1];

        EV <<"nextSatId: " << nextSatId <<endl;

        int leftSatId = sat->hasLeftSat() ? sat->getLeftSatId() : -1;
        int upSatId = sat->hasUpSat() ? sat->getUpSatId() : -1;
        int rightSatId = sat->hasRightSat() ? sat->getRightSatId() : -1;
        int downSatId = sat->hasDownSat() ? sat->getDownSatId() : -1;

        EV <<"leftSatId: " << leftSatId <<endl;
        EV <<"upSatId: " << upSatId <<endl;
        EV <<"rightSatId: " << rightSatId <<endl;
        EV <<"downSatId: " << downSatId <<endl;

        if (nextSatId == leftSatId) {
            setRoute(dstSat, ISLDirection::LEFT);
            sender->sendMessage(pkt, ISLDirection::LEFT, false);

        } else if (nextSatId == upSatId) {
            setRoute(dstSat, ISLDirection::UP);
            sender->sendMessage(pkt, ISLDirection::UP, false);

        } else if (nextSatId == rightSatId) {
            setRoute(dstSat, ISLDirection::RIGHT);
            sender->sendMessage(pkt, ISLDirection::RIGHT, false);

        } else if (nextSatId == downSatId) {
            setRoute(dstSat, ISLDirection::DOWN);
            sender->sendMessage(pkt, ISLDirection::DOWN, false);

        } else {
            EV << "is not connected to expected next hoppppp"<<endl;
            error("%d is not connected to expected next hop %d", satId, nextSatId);
        }
    }
}


void Sat_Net_DDRARouting::handleMaxHopsReached(Packet *pkt) {
    auto header = pkt->peekAtFront<routing::ddra::DDRARoutingHeader>();
    if (header->getType() == PacketType::NORMAL) {
        sender->dropPacket(pkt, PacketDropReason::HOP_LIMIT_REACHED, false);
    } else {
        handleControlPacket(header);
        delete pkt;
    }
}

void Sat_Net_DDRARouting::handleControlPacket(inet::Ptr<const routing::ddra::DDRARoutingHeader> header) {
    int srcSat = header->getSrcSat();

    if (header->getType() == PacketType::CONGESTED) {
        // update state of satellite
        congestedSats.emplace(srcSat);

        // debug logs
#ifndef NDEBUG
        EV_DEBUG << "Satellite " << satId << " received congested notification from " << srcSat << endl;
        ////EV_DEBUG << " -> " << satId << "'s known congested sats: " << flora::core::utils::set::toString(congestedSats.begin(), congestedSats.end()) << endl;
#endif
    } else if (header->getType() == PacketType::FAILURE) {
        EV << "PacketType::FAILURE" <<endl;
        int targetSat = header->getFailureTarget();
        ASSERT(targetSat != -1);

#ifndef NDEBUG
        EV_DEBUG << satId << " received that " << srcSat << " has no longer a connection to " << targetSat << endl;
#endif

        FailedLink fLink = createFailedLink(satId, targetSat);
        failedLinks.emplace_back(fLink);
    } else if (header->getType() == PacketType::UNCONGESTED) {
        congestedSats.erase(srcSat);

        // debug logs
#ifndef NDEBUG
        EV_DEBUG << "Satellite " << satId << " received uncongested notification from " << srcSat << endl;
        ////EV_DEBUG << " -> " << satId << "'s known congested sats: " << flora::core::utils::set::toString(congestedSats.begin(), congestedSats.end()) << endl;
#endif
    } else {
        error("Unexpected type");
    }
    knownUnroutableSats.clear();
    applyDynamicRoutes();
    clearRoutes();
}


void Sat_Net_DDRARouting::handlePacketDrop(Packet *pkt, PacketDropReason reason) {
    Enter_Method("handlePacketDrop");
    if (reason != PacketDropReason::INTERFACE_DOWN) {
        return;
    }

    ISLDirection direction = pkt->getTag<SendOnTag>()->getDir();

    int dropId = -1;
    switch (direction) {
        case ISLDirection::LEFT: {
            leftDrops += 1;
            if (leftDrops == 3 && sat->hasLeftSat()) {
                dropId = sat->getLeftSatId();
            }
        } break;
        case ISLDirection::UP: {
            upDrops += 1;
            if (upDrops == 3 && sat->hasUpSat()) {
                dropId = sat->getUpSatId();
            }
        } break;
        case ISLDirection::RIGHT: {
            rightDrops += 1;
            if (rightDrops == 3 && sat->hasRightSat()) {
                dropId = sat->getRightSatId();
            }
        } break;
        case ISLDirection::DOWN: {
            downDrops += 1;
            if (downDrops == 3 && sat->hasDownSat()) {
                dropId = sat->getDownSatId();
            }
        } break;
        case ISLDirection::GROUNDLINK: {
            return;
        } break;
        default:
            break;
    }

    if (dropId != -1) {
        FailedLink fLink = createFailedLink(satId, dropId);
        failedLinks.emplace_back(fLink);

        // broadcast to neighbor satellites
        createControlPacket(PacketType::FAILURE, dropId);
    }
}

void Sat_Net_DDRARouting::handleQueueSize(int queueSize, int maxQueueSize) {
    Enter_Method("handleQueueSize");

    if (queueSize > queueThreshold && !congestedMsgSent) {
        congestedMsgSent = true;
        createControlPacket(PacketType::CONGESTED);
        // inet::Packet *controlPacket = createControlPacket(PacketType::CONGESTED);
        // broadcastPacket(controlPacket);
    } else if (queueSize == 0 && congestedMsgSent) {
        congestedMsgSent = false;
        createControlPacket(PacketType::UNCONGESTED);
    }
}

void Sat_Net_DDRARouting::createControlPacket(PacketType type, int failureTargetSat) {
    if (type == PacketType::NORMAL)
        error("DDRARouting::createControlPacket: PacketType::NORMAL not allowed!");

    const char *name = nullptr;
    if (type == PacketType::CONGESTED) {
        name = "DDRA-CONGESTED";
    } else if (type == PacketType::FAILURE) {
        name = "DDRA-FAILURE";
    } else if (type == PacketType::UNCONGESTED) {
        name = "DDRA-UNCONGESTED";
    }
    EV << "DDRARouting::createControlPacket"<< endl;
    auto header = makeShared<DDRARoutingHeader>();
    header->setSrcSat(satId);
    header->setDstSat(-2);
    header->setDstGs(-2);
    header->setType(type);
    header->setFailureTarget(failureTargetSat);
    header->setChunkLength(B(20));

    // satellites
    if (sat->hasLeftSat() && leftDrops < 3) {
        inet::Packet *pkt = createControlPacketBase(name, sat->getLeftSatId(), -1, 1);
        pkt->insertAtFront(header->dupShared());
        sender->sendMessage(pkt, ISLDirection::LEFT, true);
    }
    if (sat->hasUpSat() && upDrops < 3) {
        inet::Packet *pkt = createControlPacketBase(name, sat->getUpSatId(), -1, 1);
        pkt->insertAtFront(header->dupShared());
        sender->sendMessage(pkt, ISLDirection::UP, true);
    }
    if (sat->hasRightSat() && rightDrops < 3) {
        inet::Packet *pkt = createControlPacketBase(name, sat->getRightSatId(), -1, 1);
        pkt->insertAtFront(header->dupShared());
        sender->sendMessage(pkt, ISLDirection::RIGHT, true);
    }
    if (sat->hasDownSat() && downDrops < 3) {
        inet::Packet *pkt = createControlPacketBase(name, sat->getDownSatId(), -1, 1);
        pkt->insertAtFront(header->dupShared());
        sender->sendMessage(pkt, ISLDirection::DOWN, true);
    }

    // groundstations
    int numGs = topologyControl->getNumberOfGroundstations();
    for (size_t gsId = 0; gsId < numGs; gsId++) {
        if (getGroundlinkIndex(satId, gsId) != -1) {
            inet::Packet *pkt = createControlPacketBase(name, -1, gsId, 1);
            header->setDstGs(gsId);
            pkt->insertAtFront(header->dupShared());
            sender->sendMessage(pkt, ISLDirection::GROUNDLINK, true, gsId);
        }
    }
}

void Sat_Net_DDRARouting::applyDynamicRoutes() {
    int numSats = topologyControl->getNumberOfSatellites();
    // apply known link failures
    for (auto it = failedLinks.begin(); it != failedLinks.end(); it++) {
        costMatrix[it->srcSat][it->dstSat] = INT_MAX;
    }

    // apply congested sats
    for (auto it = congestedSats.begin(); it != congestedSats.end(); it++) {
        costMatrix[satId][*it] = INT_MAX;
    }
}

void Sat_Net_DDRARouting::finish(){
}

void Sat_Net_DDRARouting::recalculateRoutes() {
    clearRoutes();

    EV << "DDRARouting::recalculateRoutes" <<endl;
    EV << "satId: " << satId << endl;
    int numSats     = topologyControl->getNumberOfSatellites();
    int leftSatId   = sat->hasLeftSat() ? sat->getLeftSatId() : -1;
    int upSatId     = sat->hasUpSat() ? sat->getUpSatId() : -1;
    int rightSatId  = sat->hasRightSat() ? sat->getRightSatId() : -1;
    int downSatId   = sat->hasDownSat() ? sat->getDownSatId() : -1;

    core::dspa::DijkstraResultFull dijkstraRes = core::dspa::runDijkstraFull(costMatrix, satId);
    for (size_t pSatId = 0; pSatId < numSats; pSatId++) {
        if (pSatId == satId) continue;  // is this sat
        auto path = core::dspa::reconstructPath(dijkstraRes, pSatId);
        if (path.size() == 0) {
            EV_DEBUG << "No route found from " << satId << " to " << pSatId << endl;
            EV << "No route found from " << satId << " to " << pSatId << endl;
            continue;
        };  // no route found
        int nextSatId = path[1];

        if (nextSatId == leftSatId) {
            setRoute(pSatId, ISLDirection::LEFT);
        } else if (nextSatId == upSatId) {
            setRoute(pSatId, ISLDirection::UP);
        } else if (nextSatId == rightSatId) {
            setRoute(pSatId, ISLDirection::RIGHT);
        } else if (nextSatId == downSatId) {
            setRoute(pSatId, ISLDirection::DOWN);
        } else {
            error("%d is not connected to expected next hop %d", satId, nextSatId);
        }
    }

    printForwardingTable();
}
}

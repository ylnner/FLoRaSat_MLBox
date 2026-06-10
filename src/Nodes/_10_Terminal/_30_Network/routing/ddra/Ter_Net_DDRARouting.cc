/*
 * Ter_Net_DDRARouting.cc
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */



#include "Ter_Net_DDRARouting.h"

using namespace routing::ddra;

namespace routing{
Define_Module(Ter_Net_DDRARouting);

void Ter_Net_DDRARouting::initialize(int stage) {
    Ter_Base_Routing::initialize(stage);
}

void Ter_Net_DDRARouting::handleTopologyChange(bool topologyChanges) {
    // Enter_Method("handleTopologyChange");
    bool mustRecalc = false;
    if (topologyChanges || !failedLinks.empty()) {
        costMatrix = ((topologycontrol::ConstellationTopologyControl*)topologyControl)->getCostMatrix();
        failedLinks.clear();

        recalculateRoutes();
        recalculateRoutesDevices();

        printAllRoutesTable();
    }
}

void Ter_Net_DDRARouting::handleTopologyChangeDevices(bool topologyChanges) {
    // Enter_Method("handleTopologyChange");
    bool mustRecalc = false;
    if (topologyChanges || !failedLinks.empty()) {
        costMatrix = ((topologycontrol::ConstellationTopologyControl*)topologyControl)->getCostMatrix();
        failedLinks.clear();

        recalculateRoutes();
        recalculateRoutesDevices();
    }
}

void Ter_Net_DDRARouting::prepareSendPacket(inet::Packet* pkt, int firstSat, int lastSat, int dstGs) {
    auto header = makeShared<routing::ddra::DDRARoutingHeader>();
    header->setSrcSat(firstSat);
    header->setDstSat(lastSat);
    header->setSrcGs(id);

    if(dstGs < 0){
        // Get the closes Gs to the Terminal
        header->setDstGs(getClosestDstId(0));
    }else{
        header->setDstGs(dstGs);
    }

    header->setType(PacketType::NORMAL);
    header->setFailureTarget(-1);
    header->setChunkLength(B(20));
    pkt->insertAtFront(header);
}

void Ter_Net_DDRARouting::receivePacket(inet::Packet* pkt) {
    EV << "Ter_Net_DDRARouting::receivePacket"<< endl;
    pkt->removeAtFront<DDRARoutingHeader>();
}

void Ter_Net_DDRARouting::handlePacket(inet::Packet* pkt) {
    EV << "Arrive to handlePacket ....../*/*/ "<<endl;

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

    auto header = pkt->peekAtFront<DDRARoutingHeader>();
    int srcSat = header->getSrcSat();

    if (header->getType() == PacketType::CONGESTED) {
        // update state of satellite
        // congestedSats.emplace(srcSat);

#ifndef NDEBUG
        EV_DEBUG << "GS " << id << " received congested notification from " << srcSat << endl;
        EV_DEBUG << " -> " << id << "'s known congested sats: " << base::set::toString(congestedSats.begin(), congestedSats.end()) << endl;
#endif

    } else if (header->getType() == PacketType::FAILURE) {
        int targetSat = header->getFailureTarget();
        ASSERT(targetSat != -1);

#ifndef NDEBUG
        EV_DEBUG << "GS " << id << " received that " << srcSat << " has no longer a connection to " << targetSat << endl;
#endif
        FailedLink fLink = createFailedLink(srcSat, targetSat);
        failedLinks.emplace_back(fLink);
    } else if (header->getType() == PacketType::UNCONGESTED) {
        congestedSats.erase(srcSat);

#ifndef NDEBUG
        EV_DEBUG << "GS " << id << " received uncongested notification from " << srcSat << endl;
        EV_DEBUG << " -> " << id << "'s known congested sats: " << base::set::toString(congestedSats.begin(), congestedSats.end()) << endl;
#endif
    }else if(header->getType() == PacketType::NORMAL){
#ifndef NDEBUG
        EV_DEBUG << "GS " << id << " received normal notification from " << srcSat << endl;
#endif

    }else {
        error("Unexpected type");
    }
    recalculateRoutes();
}

/*
std::pair<int, int> Ter_Net_DDRARouting::calculateFirstAndLastSatellite(int dstGs) {
    EV << "Execute Ter_Net_DDRARouting::calculateFirstAndLastSatellite"<<endl;
    printForwardingTable();
    return getHop(dstGs);
}

std::pair<int, int> Ter_Net_DDRARouting::calculateFirstAndLastSatelliteToDevice(int dstDev) {
    EV << "Execute Ter_Net_DDRARouting::calculateFirstAndLastSatelliteToDevice"<<endl;
    EV << "dstDev: " << dstDev <<endl;
    printForwardingTable();
    return getHop(dstDev, false);
}
*/

void Ter_Net_DDRARouting::recalculateRoutes() {
    EV << "Ter_Net_DDRARouting::recalculateRoutes()" <<endl;

    clearRoutesGS();

    int numGs = topologyControl->getNumberOfGroundstations();
    EV << "numGs: " << numGs << endl;
    EV << "id: " << id << endl;

    for (size_t dstGs = 0; dstGs < numGs; dstGs++) {
        if(!isTerminal){
            if (dstGs == id) continue;  // is this gs
        }

        //if (dstGs == id) continue;  // is this gs
        EV_DEBUG << "<><><><><><><><>" << endl;
        EV_DEBUG << "Calculate first and last sat. Distances " << id << "->" << dstGs << ":" << endl;

        Sta* dstGsInfo = topologyControl->getGroundstationInfo(dstGs);

        // calculate potential first and last sat
        int firstSat     = -1;
        int lastSat      = -1;
        double cDistance = INT_MAX;

        const std::set<int>& total_satellites_ref = isTerminal ? ld->getSatellites() : gs->getSatellites();
        int dstGsOrDev = isTerminal ? ld->getDstGsOrDev() : gs->getDstGsOrDev();

        //const std::set<int>& total_satellites_ref = gs->getSatellites();

        //for (int pFirstSat : gs->getSatellites()) {
        for (int pFirstSat : total_satellites_ref) {
            if (base::set::contains(congestedSats, pFirstSat)) continue;  // congested sat
            EV << "pFirstSat: "<< pFirstSat <<endl;

            auto pLastSats = dstGsInfo->getSatellites();
            std::set<int> stillConnectedSats;
            simtime_t maxPacketDelay = SimTime(200, SimTimeUnit::SIMTIME_MS);
            for (int pLastSat : pLastSats) {
                EV << "pLastSat: "<< pLastSat <<endl;
                if (!topologyControl->isStillConnectedAt(dstGs, pLastSat, maxPacketDelay)) {
                    // is no longer connected to gs
                    EV_DEBUG << "Route will not be valid for the next 200ms" << endl;
                    continue;
                } else {
                    stillConnectedSats.insert(pLastSat);
                }
            }

            // if there is no sat, check future sats
            if (stillConnectedSats.size() == 0) {
                EV_DEBUG << "No current found satellite is available as last sat, compute another" << endl;
                EV << "No current found satellite is available as last sat, compute another" << endl;
                simtime_t avgReceptionTime = SimTime(50, SimTimeUnit::SIMTIME_MS);
                for (size_t pSat = 0; pSat < topologyControl->getNumberOfSatellites(); pSat++) {
                    if (topologyControl->isStillConnectedAt(dstGs, pSat, avgReceptionTime)) {
                        stillConnectedSats.insert(pSat);
                    }
                }
                if (stillConnectedSats.size() == 0) continue;  // still none found, unreachable
            }

            core::dspa::DijkstraResultEarlyAbort result = core::dspa::runDijkstraEarlyAbort(costMatrix, pFirstSat, stillConnectedSats);

            int distance = result.nodes[result.dst].distance;

            EV_DEBUG << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            //EV << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;

            // ACHF
            setAllRoutes(dstGs, pFirstSat, result.dst);


            if (distance < cDistance) {
                firstSat = pFirstSat;
                lastSat = result.dst;
                cDistance = distance;
                EV_DEBUG << "Set as new shortest path" << endl;
                //EV << "Set as new shortest path" << endl;
            }
        }
        if (firstSat != -1 && lastSat != -1) {
            setRoute(dstGs, firstSat, lastSat);
            setDistance(dstGs, cDistance, 0);
        }
    }
}

void Ter_Net_DDRARouting::recalculateRoutesDevices() {
    EV << "Ter_Net_DDRARouting::recalculateRoutesDevices()" <<endl;
    EV << "isTerminal: " << isTerminal <<endl;
    if(isTerminal)
        EV << "Es lora node"<<endl;
    else
        EV << "Es ground station"<<endl;

    //clearRoutes();
    clearRoutesDevices();

    //int numGs = topologyControl->getNumberOfGroundstations();
    int numDevices = topologyControl->getNumberOfDevices();
    EV << "numDevices: " << numDevices <<endl;

    EV << "id: " << id << endl;

    for (size_t dstDev = 0; dstDev < numDevices; dstDev++) {
        EV << "....../*/*/ "<<endl;
        EV << "isTerminal: " << isTerminal <<endl;

        //if (dstGs == id) continue;  // is this gs

        EV_DEBUG << "<><><><><><><><>" << endl;
        EV_DEBUG << "Calculate first and last sat. Distances " << id << "->" << dstDev << ":" << endl;

        if(isTerminal){
            if(dstDev == id) continue;
        }

        Ter* deviceDstInfo = topologyControl->getDeviceInfo(dstDev);

        // calculate potential first and last sat
        int firstSat     = -1;
        int lastSat      = -1;
        double cDistance = INT_MAX;
        EV << "isTerminal: " << isTerminal <<endl;

        const std::set<int>& total_satellites_ref = isTerminal ? ld->getSatellites() : gs->getSatellites();
        int dstGsOrDev = isTerminal ? ld->getDstGsOrDev() : gs->getDstGsOrDev();

        for (int pFirstSat : total_satellites_ref) {
            if (base::set::contains(congestedSats, pFirstSat)) continue;  // congested sat
            EV << "pFirstSat: "<< pFirstSat <<endl;

            auto pLastSats = deviceDstInfo->getSatellites();
            std::set<int> stillConnectedSats;
            simtime_t maxPacketDelay = SimTime(200, SimTimeUnit::SIMTIME_MS);
            for (int pLastSat : pLastSats) {
                EV << "pLastSat: "<< pLastSat <<endl;
                if (!topologyControl->isStillConnectedAt(dstDev, pLastSat, maxPacketDelay, false)) {
                    // is no longer connected to gs
                    EV_DEBUG << "Route will not be valid for the next 200ms" << endl;
                    continue;
                } else {
                    stillConnectedSats.insert(pLastSat);
                }
            }

            // if there is no sat, check future sats
            if (stillConnectedSats.size() == 0) {
                EV_DEBUG << "No current found satellite is available as last sat, compute another" << endl;
                EV << "No current found satellite is available as last sat, compute another" << endl;
                simtime_t avgReceptionTime = SimTime(50, SimTimeUnit::SIMTIME_MS);
                for (size_t pSat = 0; pSat < topologyControl->getNumberOfSatellites(); pSat++) {
                    if (topologyControl->isStillConnectedAt(dstDev, pSat, avgReceptionTime, false)) {
                        stillConnectedSats.insert(pSat);
                    }
                }
                if (stillConnectedSats.size() == 0) continue;  // still none found, unreachable
            }

            core::dspa::DijkstraResultEarlyAbort result = core::dspa::runDijkstraEarlyAbort(costMatrix, pFirstSat, stillConnectedSats);

            int distance = result.nodes[result.dst].distance;

            EV_DEBUG << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            //EV << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;

            if (distance < cDistance) {
                firstSat = pFirstSat;
                lastSat = result.dst;
                cDistance = distance;
                EV_DEBUG << "Set as new shortest path" << endl;
                //EV << "Set as new shortest path" << endl;
            }
        }
        if (firstSat != -1 && lastSat != -1) {
            setRoute(dstDev, firstSat, lastSat, false);
            setDistance(dstDev, cDistance, 1); //dstGsOrDev = 1
        }
    }
}

}

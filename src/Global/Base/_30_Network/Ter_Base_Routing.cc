/*
 * Ter_Ter_Base_Routing.cc
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */


#include "Ter_Base_Routing.h"

using namespace base::set;
using namespace base;

namespace routing {
Define_Module(Ter_Base_Routing);

void Ter_Base_Routing::initialize(int stage){

    if (stage == inet::INITSTAGE_LOCAL) {
        topologyControl = check_and_cast<topologycontrol::TopologyControlBase*>(getSystemModule()->getSubmodule("topologyControl"));

        gs = dynamic_cast<Sta*>(getParentModule());

        if(!gs){
            EV << "It's not GroundStation"<<endl;
            //gs = dynamic_cast<LoRaNode*>(getParentModule());
            ld = dynamic_cast<Ter*>(getParentModule()->getParentModule());
            if(!ld){
                throw cRuntimeError("No Terminal or Station.");
            }else{
                EV << "It is a device"<<endl;
                isTerminal = true;
                id = ld->getIndex();
            }
        }else{
            //gs = dynamic_cast<ground::GroundStationRoutingBase*>(getParentModule());
            EV << "It's GS"<<endl;
            id = gs->getIndex();
        }

    }

}

int Ter_Base_Routing::getClosestDstId(int dstGsOrDev) {
    double cDistance = INT_MAX;
    int shortId      = -1;
    EV << "RoutingBaseGs::getClosestDstId" <<endl;
    EV << "dstGsOrDev: " << dstGsOrDev <<endl;
    std::unordered_map<int, double> aux_routes = getDistancesRoutes(dstGsOrDev);

    EV << "-- Distances table --\n";
    EV << ::inet::utils::stringf("%-10s %-10s\n",
                                       "Destination", "Distance");

    for (auto const& x:aux_routes){
        EV << ::inet::utils::stringf("%-10d %-10g\n", x.first, x.second);
        double distance = x.second;
        if (distance < cDistance) {
            cDistance = distance;
            shortId   = x.first;
        }
    }
    return shortId;

}

/*
std::pair<int, int> Ter_Base_Routing::calculateFirstAndLastSatellite(int dstGs) {
    EV << "Execute RoutingBaseGs::calculateFirstAndLastSatellite"<<endl;
    groundRoutingTable->printForwardingTable();

    return groundRoutingTable->getHop(dstGs);
}

std::pair<int, int> Ter_Base_Routing::calculateFirstAndLastSatelliteToDevice(int dstDev) {
    EV << "Execute RoutingBaseGs::calculateFirstAndLastSatelliteToDevice"<<endl;
    groundRoutingTable->printForwardingTable();
    return groundRoutingTable->getHop(dstDev, false);


}
*/

void Ter_Base_Routing::handleTopologyChange(bool topologyChanges) {
    Enter_Method("handleTopologyChange");

    if (topologyChanges) {
        EV << "Inside if topologyChanges"<< endl;
        recalculateRoutes();
        recalculateRoutesDevices();

        printAllRoutesTable();
    }
}

void Ter_Base_Routing::handleTopologyChangeDevices(bool topologyChanges) {
    Enter_Method("handleTopologyChangeDevices");
    EV << "RoutingBaseGs::handleTopologyChangeDevices"<<endl;
    if (topologyChanges) {
        EV << "Inside if topologyChanges"<< endl;
        recalculateRoutes();
        recalculateRoutesDevices();
    }
}


void Ter_Base_Routing::recalculateRoutes() {
    EV << "RoutingBaseGs::recalculateRoutes()" <<endl;
    //EV << "id: "<< id <<endl;

    if(isTerminal)
        EV << "It's lora node"<<endl;
    else
        EV << "It's ground station"<<endl;

    int numSats = topologyControl->getNumberOfSatellites();
    std::vector<std::vector<int>> costMatrix = routing::core::dspa::buildShortestPathCostMatrix(numSats, topologyControl->getSatellites());

    //clearRoutes();
    clearRoutesGS();
    int numGs = topologyControl->getNumberOfGroundstations();
    EV << "numGs: " << numGs <<endl;
    for (size_t dstGs = 0; dstGs < numGs; dstGs++) {
        if(!isTerminal){
            if (dstGs == id) continue;  // is this gs
        }
        EV << "<><><><><><><><>" << endl;
        EV   << "Calculate first and last sat. Distances " << id << "->" << dstGs << ":" << endl;

        Sta* dstGsInfo = topologyControl->getGroundstationInfo(dstGs);
        // calculate potential first and last sat
        int firstSat = -1;
        int lastSat = -1;
        double cDistance = INT_MAX;

        EV << "isTerminal: " << isTerminal <<endl;


        const std::set<int>& total_satellites_ref = isTerminal ? ld->getSatellites() : gs->getSatellites();
        int dstGsOrDev = isTerminal? ld->getDstGsOrDev() : gs->getDstGsOrDev();

        for (int pFirstSat : total_satellites_ref) {
            //int pFirstSat = satId;
            auto pLastSats = dstGsInfo->getSatellites();
            EV << "pFirstSat: "<< pFirstSat <<endl;
            //EV << "pLastSats: "<< pLastSats <<endl;
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

            routing::core::dspa::DijkstraResultEarlyAbort result = routing::core::dspa::runDijkstraEarlyAbort(costMatrix, pFirstSat, stillConnectedSats);

            int distance = result.nodes[result.dst].distance;

            EV_DEBUG << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            EV << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            if (distance < cDistance) {
                firstSat = pFirstSat;
                lastSat = result.dst;
                cDistance = distance;
                EV_DEBUG << "Set as new shortest path" << endl;
                EV << "Set as new shortest path" << endl;
            }
        }
        if (firstSat != -1 && lastSat != -1) {
            EV << "Before SetRoute RoutinBaseGS" << endl;
            EV << "dstGs: " << dstGs<<endl;
            EV << "firstSat: " << firstSat<<endl;
            EV << "lastSat: " << lastSat<<endl;
            setRoute(dstGs, firstSat, lastSat);
            EV << "After SetRoute RoutingBaseGS"<<endl;
            EV << "cDistance: " << cDistance <<endl;
            if(cDistance > 2000){
                EV << "setDistance id: " << id <<endl;
                setDistance(dstGs, cDistance, 0); //dstGsOrDev = 0
                EV << "After SetDistances RoutingBaseGS"<<endl;
                //printForwardingTable();
            }
        }
    }
}


/// The destination is a device
void Ter_Base_Routing::recalculateRoutesDevices() {
    EV << "RoutingBaseGs::recalculateRoutesDevices()" <<endl;

    int numSats = topologyControl->getNumberOfSatellites();
    std::vector<std::vector<int>> costMatrix = routing::core::dspa::buildShortestPathCostMatrix(numSats, topologyControl->getSatellites());
    //clearRoutes();
    clearRoutesDevices();

    int numDevices = topologyControl->getNumberOfDevices();
    EV << "numDevices: " << numDevices <<endl;

    for (size_t dstDev = 0; dstDev < numDevices; dstDev++) {

        EV_DEBUG << "<><><><><><><><>" << endl;
        EV_DEBUG << "Calculate first and last sat. Distances " << id << "->" << dstDev << ":" << endl;

        EV << "<><><><><><><><>" << endl;
        EV   << "Calculate first and last sat. Distances " << id << "->" << dstDev << ":" << endl;

        // Revisar que esto sigue, dado que ahora se ejecuta en el satelite
        if(isTerminal){
            if (dstDev == id) continue;  // is this gs
        }
        Ter* deviceDstInfo = topologyControl->getDeviceInfo(dstDev);

        // calculate potential first and last sat
        int firstSat = -1;
        int lastSat = -1;
        double cDistance = INT_MAX;

        const std::set<int>& total_satellites_ref = isTerminal ? ld->getSatellites() : gs->getSatellites();
        int dstGsOrDev = isTerminal? ld->getDstGsOrDev() : gs->getDstGsOrDev();


        for (int pFirstSat : total_satellites_ref) {
            ///int pFirstSat = satId;
            auto pLastSats = deviceDstInfo->getSatellites();

            std::set<int> stillConnectedSats;
            simtime_t maxPacketDelay = SimTime(200, SimTimeUnit::SIMTIME_MS);

            for (int pLastSat : pLastSats) {

                EV << "====== pLastSat: "<< pLastSat <<endl;
                if (!topologyControl->isStillConnectedAt(dstDev, pLastSat, maxPacketDelay, false)) {
                    // is no longer connected to gs
                    EV_DEBUG << "Route will not be valid for the next 200ms" << endl;
                    EV << "Route will not be valid for the next 200ms" << endl;
                    continue;
                } else {
                    EV << "stillConnectedSats" <<endl;
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

            routing::core::dspa::DijkstraResultEarlyAbort result = routing::core::dspa::runDijkstraEarlyAbort(costMatrix, pFirstSat, stillConnectedSats);

            int distance = result.nodes[result.dst].distance;

            EV_DEBUG << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            EV << "Distance between " << pFirstSat << "->" << result.dst << ": " << distance << endl;
            if (distance < cDistance) {
                firstSat = pFirstSat;
                lastSat = result.dst;
                cDistance = distance;
                EV_DEBUG << "Set as new shortest path" << endl;
                EV << "Set as new shortest path" << endl;
            }
        }

        if (firstSat != -1 && lastSat != -1) {
            setRoute(dstDev, firstSat, lastSat, false);
            /*
             * TO DO: Ensures a minimum distance is maintained for successful message transmission
             * */
            if (cDistance > 2000){
                setDistance(dstDev, cDistance, 1); ////dstGsOrDev = 1
            }
        }
    }
}

}  // namespace routing

/*
 * ForwardingTable.cc
 *
 *  Created on: Jun 01, 2023
 *      Author: Robin Ohs
 */

#include "GroundForwardingTable.h"
////Define_Module(GroundForwardingTable);

GroundForwardingTable::GroundForwardingTable() {

}

GroundForwardingTable::~GroundForwardingTable() {
}

void GroundForwardingTable::printForwardingTable() const {
// ACHF Needs to be activated only for debug
//#ifndef NDEBUG
    EV << "-- Forwarding table --\n";
    EV << ::inet::utils::stringf("%-10s %-10s %-10s\n",
                                       "Destination", "FirstSat", "LastSat");

    for (auto const& x : routes) {
        auto dst = x.first;
        auto out1 = x.second.first;
        auto out2 = x.second.second;
        EV << ::inet::utils::stringf("%-10d %-10d %-10d\n", dst, out1, out2);
    }
    EV << "\n";

    EV << "-- -- Devices -- --\n";
    EV << ::inet::utils::stringf("%-10s %-10s %-10s\n",
                                       "Destination", "FirstSat", "LastSat");

    for (auto const& x : routesDevices) {
        auto dst = x.first;
        auto out1 = x.second.first;
        auto out2 = x.second.second;
        EV << ::inet::utils::stringf("%-10d %-10d %-10d\n", dst, out1, out2);
    }
    EV << "\n";

    EV << "-- Distances table routes distance --\n";
    EV << ::inet::utils::stringf("%-10s %-10s\n",
                                       "Destination", "Distance");

    for (auto const& x:routesDistance){
        EV << ::inet::utils::stringf("%-10d %-10g\n", x.first, x.second);
    }

    EV << "\n";
    EV << "-- Distances table routes distance device --\n";
    EV << ::inet::utils::stringf("%-10s %-10s\n",
                                       "Destination", "Distance");

    for (auto const& x:routesDistanceDevices){
        EV << ::inet::utils::stringf("%-10d %-10g\n", x.first, x.second);
    }
//#endif
}

std::unordered_map<int, double> GroundForwardingTable::getDistancesRoutes(int dstGsOrDev){
    EV << "GroundForwardingTable::getDistancesRoutes" <<endl;
    if(dstGsOrDev == 0)
        return routesDistance;
    else
        return routesDistanceDevices;
}

void GroundForwardingTable::setDistance(int dstId, double distance, int dstGsOrDev){
    EV << "GroundForwardingTable::setDistance "<<endl;
    EV << "dstId: " << dstId << endl;
    EV << "distance: " << distance << endl;
    EV << "dstGsOrDev: " << dstGsOrDev <<endl;

    if(distance > 2000){
        if(dstGsOrDev == 0){
            EV << "dstGsOrDev == 0" <<endl;
            routesDistance.emplace(dstId, distance);
        }else if(dstGsOrDev == 1){
            EV << "dstGsOrDev == 1" <<endl;
            routesDistanceDevices.emplace(dstId, distance);
        }
    }
}


void GroundForwardingTable::setRoute(int gsId, int firstSat, int lastSat) {
    EV << "GroundForwardingTable::setRoute "<<endl;
    EV << "gsId: "<< gsId<< endl;
    EV << "firstSat: "<< firstSat<< endl;
    EV << "lastSat: "<< lastSat<<endl;
    routes.emplace(gsId, std::make_pair(firstSat, lastSat));
}


void GroundForwardingTable::setRoute(int srcId, int firstSat, int lastSat, bool isGS){
    EV << "GroundForwardingTable::setRoute 2"<<endl;
    EV << "srcId: "<< srcId<< endl;
    EV << "firstSat: "<< firstSat<< endl;
    EV << "lastSat: "<< lastSat<<endl;
    EV << "isGS: "<< isGS<<endl;

    if(isGS){
        EV << "setRoute routes"<<endl;
        routes.emplace(srcId, std::make_pair(firstSat, lastSat));
    }else{
        EV << "setRoute routesDevices"<<endl;
        routesDevices.emplace(srcId, std::make_pair(firstSat, lastSat));
    }
}

void GroundForwardingTable::setAllRoutes(int gsId, int firstSat, int lastSat){
    EV << "GroundForwardingTable::setTotalRoute "<<endl;
    allRoutes[gsId][firstSat] = lastSat;
}

void GroundForwardingTable::setAllRoutesDevices(int gsId, int firstSat, int lastSat){
    EV << "GroundForwardingTable::setTotalRouteDevices "<<endl;
    allRoutesDevices[gsId][firstSat] = lastSat;
}

int GroundForwardingTable::getLastSat(int gsId, int firstSat) {
    printAllRoutesTable();

    try {
        return allRoutes.at(gsId).at(firstSat);
    }catch (const std::out_of_range& oore) {
        return -1;
        //throw routing::core::NoSuchRouteException(gsId);
    }


}

void GroundForwardingTable::printAllRoutesTable() {
    EV << "--- Ground Forwarding All Table Content ---" << endl;

    if (allRoutes.empty()) {
        EV << "All routes table is empty." << endl;
        return;
    }

    for (auto const& [gsId, subMap] : allRoutes) {
        EV << "Dst (GS ID): " << gsId << " | " << subMap.size() << " routes availables:" << endl;

        for (auto const& [firstSat, lastSat] : subMap) {
            EV << "  -> (FirstSat): " << firstSat
               << " | (LastSat): " << lastSat << endl;
        }
        EV << "---------------------------------------" << endl;
    }
}

void GroundForwardingTable::removeRoute(int gsId, bool isGS) {
    if(isGS){
        routes.erase(gsId);
    }else{
        routesDevices.erase(gsId);
    }

}

void GroundForwardingTable::clearRoutes() {
    routes.clear();
    routesDevices.clear();

    routesDistance.clear();
    routesDistanceDevices.clear();
    /*if (isGS){
        routes.clear();
    }else{
        routesDevices.clear();
    }
    */
}
void GroundForwardingTable::clearRoutesGS() {
    routes.clear();
    routesDistance.clear();

    allRoutes.clear();
    allRoutesDevices.clear();
}

void GroundForwardingTable::clearRoutesDevices() {
    routesDevices.clear();
    routesDistanceDevices.clear();
}

std::pair<int, int> GroundForwardingTable::getHop(int gsId, bool isDstGS) {
    EV << "GroundForwardingTable::getHop(int gsId): " << gsId <<endl;

    //EV << getParentModule()->par("sendData").boolValue() <<endl;
    try {
        if (isDstGS){
            return routes.at(gsId);
        }else{
            return routesDevices.at(gsId);
        }

    } catch (const std::out_of_range& oore) {
        throw routing::core::NoSuchRouteException(gsId);
    }
}



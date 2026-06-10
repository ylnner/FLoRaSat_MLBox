/*
 * ForwardingTable.cc
 *
 *  Created on: Jun 01, 2023
 *      Author: Robin Ohs
 */

#include "ForwardingTable.h"


ForwardingTable::ForwardingTable() {
}

ForwardingTable::~ForwardingTable() {
}

void ForwardingTable::printForwardingTable() const {
// ACHF Needs to be activated only for debug
//#ifndef NDEBUG
    EV << "-- Forwarding table --\n";
    EV << ::inet::utils::stringf("%-10s %-10s\n",
                                       "Destination", "Out");

    for (auto const& x : routes) {
        auto dst = x.first;
        auto out = to_string(x.second);
        EV << ::inet::utils::stringf("%-10d %-10s\n", dst, out.c_str());
    }
    EV << "\n";
//#endif
}

void ForwardingTable::setRoute(int satId, isldirection::ISLDirection dir) {
    EV << "ForwardingTable::setRoute"<<endl;
    EV << "satId: " << satId<<endl;
    EV << "dir: " << dir<<endl;
    routes.emplace(satId, dir);
}

void ForwardingTable::removeRoute(int satId) {
    routes.erase(satId);
}

void ForwardingTable::clearRoutes() {
    routes.clear();
}

isldirection::ISLDirection ForwardingTable::getNextHop(int satId) {
    EV << "\nForwardingTable::getNextHop"<<endl;
    EV << "satId: "<< satId<<endl;
    printForwardingTable();
    try {
        return routes.at(satId);
    } catch (const std::out_of_range& oore) {
        EV << "Into catch"<<endl;
        throw routing::core::NoSuchRouteException(satId);
    }
}

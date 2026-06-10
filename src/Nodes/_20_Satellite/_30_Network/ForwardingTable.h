/*
 * ForwardingTable.h
 *
 *  Created on: Jun 01, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_FORWARDINGTABLE_H_
#define __FLORA_ROUTING_FORWARDINGTABLE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "Global/Utilities/ISLDirection.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "Global/Utilities/Routing_Core/NoSuchRouteException.h"

using namespace omnetpp;
using namespace inet;
using namespace core;
//using namespace flora::core;


class ForwardingTable {
   public:
    ForwardingTable();
    ~ForwardingTable();

    /** @brief Prints the forwarding table for debugging purposes. */
    void printForwardingTable() const;

    void setRoute(int satId, core::isldirection::ISLDirection dir);
    void removeRoute(int satId);
    void clearRoutes();

    /**
     * @brief Gets the next hop for the given satId.
     *
     * @throws NoSuchRouteException: if there is no route to the given satId.
     */
    core::isldirection::ISLDirection getNextHop(int satId);

   private:
    std::unordered_map<int, core::isldirection::ISLDirection> routes;
};

#endif  // __FLORA_ROUTING_FORWARDINGTABLE_H_

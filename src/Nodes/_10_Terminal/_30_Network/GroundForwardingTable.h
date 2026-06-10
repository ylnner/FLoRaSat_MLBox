/*
 * GroundForwardingTable.h
 *
 *  Created on: Aug 30, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_GROUNDFORWARDINGTABLE_H_
#define __FLORA_ROUTING_GROUNDFORWARDINGTABLE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "Global/Utilities/ISLDirection.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "Global/Utilities/Routing_Core/NoSuchRouteException.h"

using namespace omnetpp;
using namespace inet;
//using namespace flora::core;

class GroundForwardingTable{
   public:
    GroundForwardingTable();
    ~GroundForwardingTable();

    /** @brief Prints the forwarding table for debugging purposes. */
    void printForwardingTable() const;


    void setRoute(int gsId, int firstSat, int lastSat);
    void setRoute(int srcId, int firstSat, int lastSat, bool isGS);
    void setDistance(int dstId, double distance, int dstGsOrDev);
    void removeRoute(int gsId, bool isGS=true);
    void clearRoutes();
    void clearRoutesDevices();
    void clearRoutesGS();

    std::unordered_map<int, double> getDistancesRoutes(int dstGsOrDev);

    void setAllRoutes(int gsId, int firstSat, int lastSat);
    void setAllRoutesDevices(int gsId, int firstSat, int lastSat);

    void printAllRoutesTable();

    int getLastSat(int gsId, int firstSat);

    /**
     * @brief Gets the first and last sat for gsId.
     *
     * @throws NoSuchRouteException: if there is no route to the given gsId.
     */
    std::pair<int, int> getHop(int gsId, bool isDstGS=true);

   private:
    std::unordered_map<int, std::pair<int, int>> routes;
    std::unordered_map<int, std::pair<int, int>> routesDevices;

    std::unordered_map<int, std::unordered_map<int, int>> allRoutes;
    std::unordered_map<int, std::unordered_map<int, int>> allRoutesDevices;

    std::unordered_map<int, double> routesDistance;
    std::unordered_map<int, double> routesDistanceDevices;

};

#endif  // __FLORA_ROUTING_GROUNDFORWARDINGTABLE_H_

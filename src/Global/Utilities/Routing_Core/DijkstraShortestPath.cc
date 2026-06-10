/*
 * DijkstraShortestPath.cc
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#include "DijkstraShortestPath.h"

namespace routing {
namespace core {
namespace dspa {

//
// DIJKSTRA FULL
//

DijkstraResultFull runDijkstraFull(const std::vector<std::vector<int>>& cost, int src) {
    const std::set<int> empty;
    auto res = detail::runDijkstra(cost, src, empty);
    std::vector<DijkstraNode> nodes = res.first;
    return DijkstraResultFull{src, nodes};
}

//////////////////////////////////////////////

//
// DIJKSTRA WITH EARLY ABORT
//

DijkstraResultEarlyAbort runDijkstraEarlyAbort(const std::vector<std::vector<int>>& cost, int src, const std::set<int>& dsts) {
    auto res = detail::runDijkstra(cost, src, dsts);
    std::vector<DijkstraNode> nodes = res.first;
    int dst = res.second;
    return DijkstraResultEarlyAbort{src, dst, nodes};  // TODO:
}

//////////////////////////////////////////////

//
// GET NEAREST ID
//

int getNearestId(const DijkstraResultFull& dijkstraResult, const std::set<int>& potentialIds) {
    // get lastSatellite with shortest distance + groundlink distance
    int nearestId = -1;
    int nearestDistance = INT_MAX;
    for (int pId : potentialIds) {
        ASSERT(pId >= 0 && pId < dijkstraResult.nodes.size());
        // add distance between sat and gs for selecting optimal last satellites
        int distance = dijkstraResult.nodes[pId].distance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestId = pId;
        }
    }
    return nearestId;
}

//////////////////////////////////////////////

//
// DIJKSTRA RECONSTRUCT PATH
//

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 * Can be used to construct paths to arbitrary destination ids.
 */
std::vector<int> reconstructPath(DijkstraResultFull& dijkstraResultFull, int dst) {
    return detail::reconstructPath(dijkstraResultFull.nodes, dijkstraResultFull.src, dst);
}

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 */
std::vector<int> reconstructPath(DijkstraResultEarlyAbort& dijkstraResultEarlyAbort) {
    return detail::reconstructPath(dijkstraResultEarlyAbort.nodes, dijkstraResultEarlyAbort.src, dijkstraResultEarlyAbort.dst);
}

//////////////////////////////////////////////

//
// COST MATRIX FUNCTIONS
//

std::vector<std::vector<int>> buildShortestPathCostMatrix(int constellationSize, const std::unordered_map<int, Sat*>& constellation) {
    std::vector<std::vector<int>> cost(constellationSize, std::vector<int>(constellationSize, INT_MAX));

    for (size_t i = 0; i < constellationSize; i++) {
        const Sat* sat = constellation.at(i);
        if (sat->hasLeftSat()) {
            cost[i][sat->getLeftSatId()] = (int)round(sat->getLeftSatDistance());
        }
        if (sat->hasUpSat()) {
            cost[i][sat->getUpSatId()] = (int)round(sat->getUpSatDistance());
        }
        if (sat->hasRightSat()) {
            cost[i][sat->getRightSatId()] = (int)round(sat->getRightSatDistance());
        }
        if (sat->hasDownSat()) {
            cost[i][sat->getDownSatId()] = (int)round(sat->getDownSatDistance());
        }
    }
    return cost;
}

bool isSatelliteConnected(const std::vector<std::vector<int>>& cost, int satId) {
    for (size_t i = 0; i < cost.size(); i++) {
        if (cost[satId][i] != INT_MAX) {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////

//
// IMPLEMENTATION DETAILS
//
namespace detail {

std::pair<std::vector<DijkstraNode>, int> runDijkstra(const std::vector<std::vector<int>>& cost, int src, const std::set<int>& dsts) {
    int size = cost.size();
    int dst = -1;

    std::vector<DijkstraNode> nodes(size);

    for (size_t i = 0; i < size; i++) {
        auto node = DijkstraNode();
        nodes.push_back(node);
    }

    nodes[src].distance = 0;

    for (size_t i = 0; i < size; i++) {
        int minValue = INT_MAX;
        int nearest = -1;
        for (size_t j = 0; j < size; j++) {
            if (!nodes[j].visited && nodes[j].distance < minValue) {
                minValue = nodes[j].distance;
                nearest = j;
            }
        }

        // stop if no next node was found
        if (nearest == -1) {
            break;
        }

        nodes[nearest].visited = true;

        // EARLY ABORT
        if (base::set::contains(dsts, nearest)) {
            dst = nearest;
            break;
        }

        for (size_t j = 0; j < size; j++) {
            // ignore visited nodes
            if (nodes[j].visited) continue;
            // ignore node pairs without edge
            if (cost[nearest][j] == INT_MAX) continue;
            int alt = minValue + cost[nearest][j];
            if (nodes[j].distance > alt) {
                nodes[j].distance = alt;
                nodes[j].prev = nearest;
            }
        }
    }

    return std::make_pair(nodes, dst);
}

std::vector<int> reconstructPath(const std::vector<DijkstraNode>& nodes, int src, int dst) {
    std::deque<int> path;
    path.push_back(dst);
    int it = dst;
    while (it != src) {
        it = nodes[it].prev;
        if(it == -1) {
            EV_DEBUG << "Route from " << src << " to " << dst << " does not exist!" << endl;
            return std::vector<int>();
        }
        path.push_front(it);
    }
    std::vector<int> v(std::make_move_iterator(path.begin()),
                       std::make_move_iterator(path.end()));
    return v;
}

}  // namespace detail

}  // namespace dspa
}  // namespace core
}  // namespace routing

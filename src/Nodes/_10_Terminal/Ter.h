/*
 * Ter.h
 *
 *  Created on: Oct 25, 2025
 *      Author: root
 */

#ifndef NODES__10_TERMINAL_TER_H_
#define NODES__10_TERMINAL_TER_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "_60_Mobility/Ter_Mob.h"
#include "Global/Utilities/Constants.h"
#include "Global/Utilities/PositionAwareBase.h"
#include "Global/Utilities/utils/SetUtils.h"
//#include "networklayer/ConstellationRoutingTable.h"
//#include "statistics/PacketRecorder.h"
//#include "ground/GroundStationRoutingBase.h"


using namespace omnetpp;
using namespace core;
using namespace mobility;

namespace terminal{
class Ter : public cSimpleModule , public PositionAwareBase{
    private:
        int loraNodeId;
        std::set<int> satellites;

    protected:
        Ter_Mob *mobility = nullptr;
       // data
       int maxHops = 25;
       int numGroundStations;
       int numLoraNodes;
       int dstGsOrDev = 0;

    public:
        const std::set<int> &getSatellites() const;
        void removeSatellite(int satId);
        void addSatellite(int satId);
        bool isConnectedTo(int satId);
        bool isConnectedToAnySat();

        int getDstGsOrDev(){return dstGsOrDev;}

        cGate *getInputGate(int index);
        cGate *getOutputGate(int index);

        virtual void handleMessage(cMessage *msg) override;

        double getLatitude() const override;
        double getLongitude() const override;
        double getAltitude() const override;

        friend std::ostream &operator<<(std::ostream &ss, const Ter &ln) {
                ss << "{";
                ss << "\"loraNodeId\": " << ln.loraNodeId << ",";
                ss << "\"satellites\": "
                   << "[";
                for (int satellite : ln.satellites) {
                    ss << satellite << ",";
                }
                ss << "],";
                ss << "}";
                return ss;
            }

    protected:
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

};
}


#endif /* NODES__10_TERMINAL_TER_H_ */

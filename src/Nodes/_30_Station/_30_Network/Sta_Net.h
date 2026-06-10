/*
 * Sta_Net.h
 *
 *  Created on: Oct 24, 2025
 *      Author: root
 */

#ifndef NODES__30_STATION__30_NETWORK_STA_NET_H_
#define NODES__30_STATION__30_NETWORK_STA_NET_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "Global/Utilities/Constants.h"
#include "Global/Utilities/PositionAwareBase.h"
#include "Global/Utilities/utils/SetUtils.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"


using namespace omnetpp;
using namespace core;
using namespace mobility;


class Sta_Net : public cSimpleModule, public PositionAwareBase{
    public:
        bool sendData;
        int dstGsOrDev;
        int getGroundStationId() const { return groundStationId; }
        cGate *getInputGate(int index);
        cGate *getOutputGate(int index);

        virtual void handleMessage(cMessage *msg) override;

        const std::set<int> &getSatellites() const;
        void removeSatellite(int satId);
        void addSatellite(int satId);
        bool isConnectedTo(int satId);
        bool isConnectedToAnySat();

        int getDstGsOrDev(){return dstGsOrDev;}

        double getLatitude() const override;
        double getLongitude() const override;
        double getAltitude() const override;

        friend std::ostream &operator<<(std::ostream &ss, const Sta_Net &gs) {
            ss << "{";
            ss << "\"groundStationId\": " << gs.groundStationId << ",";
            ss << "\"satellites\": "
               << "[";
            for (int satellite : gs.satellites) {
                ss << satellite << ",";
            }
            ss << "],";
            ss << "}";
            return ss;
        }

    protected:
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    private:
        int groundStationId;
        std::set<int> satellites;
        Sta_Mob_StationMobility *mobility;
};


#endif /* NODES__30_STATION__30_NETWORK_STA_NET_H_ */

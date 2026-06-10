#ifndef NODES__30_STATION_STA_H_
#define NODES__30_STATION_STA_H_

#include <omnetpp.h>
#include <set>
#include <sstream>

#include "Global/Utilities/Constants.h"
#include "Global/Utilities/PositionAwareBase.h"
#include "Global/Utilities/utils/SetUtils.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"
#include "Nodes/_30_Station/_30_Network/Sta_Net.h"
#include "Global/Utilities/LayerForwarder.h"

using namespace omnetpp;
using namespace core;
using namespace mobility;
using namespace inet;

namespace station{
class Sta : public cModule, public PositionAwareBase {
    public:
        bool sendData;
        int dstGsOrDev;
        int getGroundStationId() const { return groundStationId; }
        cGate *getInputGate(int index);
        cGate *getOutputGate(int index);

        const std::set<int> &getSatellites() const;
        void removeSatellite(int satId);
        void addSatellite(int satId);
        bool isConnectedTo(int satId);
        bool isConnectedToAnySat();

        int getDstGsOrDev(){return dstGsOrDev;}

        double getLatitude() const override;
        double getLongitude() const override;
        double getAltitude() const override;

        friend std::ostream &operator<<(std::ostream &ss, const Sta &gs) {
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
        Sta_Net *net;
        cSimpleModule *phy;
};

}

#endif /* NODES__30_STATION_STA_H_ */

/*
 * TopologyControlBase.h
 *
 *  Created on: Oct 14, 2025
 *      Author: root
 */

#ifndef GLOBAL_TOPOLOGY_CONTROL_TOPOLOGYCONTROLBASE_H_
#define GLOBAL_TOPOLOGY_CONTROL_TOPOLOGYCONTROLBASE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "Global/Utilities/Utils.h"
#include "Global/Utilities/WalkerType.h"

#include "inet/common/clock/ClockUserModuleMixin.h"
#include "Global/Utilities/ISLDirection.h"
#include "Nodes/_20_Satellite/Sat.h"
#include "Global/Topology_Control/data/GsSatConnection.h"
#include "Global/Topology_Control/utilities/ChannelState.h"


#include "Nodes/_10_Terminal/Ter.h"
#include "Nodes/_30_Station/Sta.h"

using namespace omnetpp;
using namespace satellite;
using namespace topologycontrol;
using namespace Global;
using namespace core::walkertype;
using namespace inet;//::physicallayer;
using namespace terminal;
using namespace satellite;
using namespace station;

namespace topologycontrol{

class TopologyControlBase : public ClockUserModuleMixin<cSimpleModule> {
    public:
        TopologyControlBase();

        virtual void initTopology() = 0;
        virtual void updateTopology() = 0;

        // Sat API
        Sat *const getSatellite(int satId) const;
        int calculateSatelliteId(int plane, int numberInPlane) const;
        Sat *const findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) const;
        std::unordered_map<int, Sat *> const &getSatellites() const;
        int getInterPlaneSpacing() const { return interPlaneSpacing; };
        int getNumberOfSatellites() const { return numSatellites; };
        /** @brief ΔΩ = 2𝜋/𝑃 in [0,2𝜋]. */
        double getRaanDelta() const { return raanDelta; };
        /** @brief ΔΦ = 2𝜋/Q in [0,2𝜋]. */
        double getPhaseDiff() const { return phaseDiff; };
        /** @brief Δ𝑓 = 2𝜋𝐹/𝑃𝑄 in [0,2𝜋[. */
        double getPhaseOffset() const { return phaseOffset; };
        int getSatsPerPlane() const { return satsPerPlane; };
        int getPlaneCount() const { return planeCount; };

        // future api
        virtual bool isStillConnectedAt(int gsId, int satId, simtime_t offset, bool isGS=true) = 0;

        // GS API
        Sta *const getGroundstationInfo(int gsId) const;
        int getNumberOfGroundstations() const {
            return numGroundStations;
        };

        Ter *const getDeviceInfo(int devId) const;
        int getNumberOfDevices() const{
            return numLoraDevices;
        }

        // Connections API
        GsSatConnection const &getGroundstationSatConnection(int gsId, int satId) const;
        GsSatConnection const &getDeviceSatConnection(int devId, int satId) const;

    protected:
        virtual ~TopologyControlBase();
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

        virtual void handleMessage(cMessage *msg) override;

        /** This method is called if the topology has changed.
         * It is only called if there is a pre-plannable topology change.
         * E.g., ISL gates that get disabled do not lead to a call.
         */
        virtual void trackTopologyChange() {}

        /** Used to connect a satellite pair with the given direction as the output direction of the first satellite.
         * The input gate is the output gate of the second satellite on the counter direction.
         */
        void connectSatellites(Sat *first, Sat *second, isldirection::ISLDirection direction);

        /** Used to connect a satellite pair with the given direction as the output direction of the first satellite.
         * The input gate is the output gate of the second satellite on the counter direction.
         */
        void disconnectSatellites(Sat *first, Sat *second, isldirection::ISLDirection direction);

        /** @brief Creates a one-way connection between the outGate and the inGate.
         * Considers the ISLStates of the gates and only connects if they are both working.
         */
        void createSatelliteConnection(cGate *outGate, cGate *inGate, double delay, double datarate, ISLState outState, ISLState inState);

        /** @brief Creates/Updates the channel from outGate to inGate. If channel exists updates channel params, otherwise creates the channel.*/
        ChannelState updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate);
        /** @brief Deletes the channel of outGate. If channel does not exist, nothing happens, otherwise deletes the channel.*/
        ChannelState deleteChannel(cGate *outGate);

    private:
        /** @brief Schedules the update timer that will update the topology state.*/
        void scheduleUpdate();
        void loadSatellites();
        void loadGroundstations();
        void loadLoraDevices();
        void resolveConstellationParametersFromNorad();
        Global::INorad *getNoradModule() const;


    protected:
        /** @brief Map of satellite ids and their correspinding SatelliteInfo data struct. */
        std::unordered_map<int, Sat *> satellites;
        int numSatellites;

        /** @brief Structs that represent groundstations and all satellites in range. */
        std::unordered_map<int, Sta *> groundStations;

        // ACHF
        std::unordered_map<int, terminal::Ter *> loraDevices;
        /////std::unordered_map<int, GroundStationRoutingBase *> groundStationsTransmitter;

        int numGroundStations            = 0;
        int numLoraDevices               = 0;
        int numGroundStationsTransmitter = 0;
        int numGroundLinks               = 60;



        std::map<std::pair<int, int>, GsSatConnection> deviceSatConnections;
        std::map<std::pair<int, int>, GsSatConnection> gsTransmitterSatConnections;

        /** @brief Structs that represent connections between satellites and groundstations. */
        std::map<std::pair<int, int>, GsSatConnection> gsSatConnections;

        /** @brief Can be set to true for constellations that do not feature intra-plane ISL*/
        bool intraPlaneIslDisabled;
        /** @brief Can be set to true for constellations that do not feature inter-plane ISL*/
        bool interPlaneIslDisabled;
        /** @brief Allows uneven plane sizes by relaxing validation and skipping missing links. */
        bool allowUnevenPlanes;
        /** @brief Delay of the isl channel, in microseconds/km. */
        double islDelay;
        /** @brief Datarate of the isl channel, in bit/second. */
        double islDatarate;

        /** @brief Delay of the groundlink channel, in microseconds/km. */
        double groundlinkDelay;
        /** @brief Datarate of the groundlink channel, in bit/second. */
        double groundlinkDatarate;
        /** @brief The minimum elevation between a satellite and a groundstation.*/
        double minimumElevation;

        WalkerType walkerType;
        double raanDelta;
        double phaseDiff;
        double phaseOffset;
        int interPlaneSpacing;
        int planeCount;
        int satsPerPlane;

        /** @brief Used to indicate if there was a change to the topology. */
        bool topologyChanged = false;

        /**
         * @brief The simulation time interval used to regularly signal mobility state changes.
         *
         * The 0 value turns off the signal.
         */
        cPar *updateIntervalParameter = nullptr;
        ClockEvent *updateTimer = nullptr;


};

}


#endif /* GLOBAL_TOPOLOGY_CONTROL_TOPOLOGYCONTROLBASE_H_ */

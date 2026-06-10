/*
 * Sat.h
 *
 *  Created on: Oct 14, 2025
 *      Author: root
 */

#ifndef NODES__20_SATELLITE_SAT_H_
#define NODES__20_SATELLITE_SAT_H_


#include <omnetpp.h>

#include "Global/Utilities/Constants.h"
#include "Global/Utilities/ISLDirection.h"
#include "Global/Utilities/ISLState.h"
#include "Global/Utilities/PositionAwareBase.h"
#include "Global/Utilities/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"

#include "Global/Mobility/INorad.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_NoradA.h"
//#include "mobility/UniformGroundMobility.h"

using namespace omnetpp;
using namespace inet;
using namespace core;
using core::Constants;
using namespace Global;
using namespace mobility;

namespace satellite {

class Sat : public cSimpleModule, public PositionAwareBase {
   protected:
    int satId = -1;
    int satPlane;
    int satNumberInPlane;
    //INorad *noradModule;
    Sat_Mob_NoradA *noradModule;

    /** @brief The upper latitude to shut down inter-plane ISL. (Noth-Pole) */
    double upperLatitudeBound;
    /** @brief The lower latitude to shut down inter-plane ISL. (South-Pole) */
    double lowerLatitudeBound;

    Sat *leftSatellite = nullptr;
    Sat *rightSatellite = nullptr;
    Sat *upSatellite = nullptr;
    Sat *downSatellite = nullptr;

    ISLState leftSendState = ISLState::WORKING;
    ISLState leftRecvState = ISLState::WORKING;
    ISLState upSendState = ISLState::WORKING;
    ISLState upRecvState = ISLState::WORKING;
    ISLState rightSendState = ISLState::WORKING;
    ISLState rightRecvState = ISLState::WORKING;
    ISLState downSendState = ISLState::WORKING;
    ISLState downRecvState = ISLState::WORKING;

   public:
    /** @brief Returns the id of the satellite. */
    int getId() const { return satId; }
    /** @brief Returns the plane of the satellite. */
    int getPlane() const { return satPlane; }
    /** @brief Gives the number in the plane. The first sat in any plane has number 0.*/
    int getNumberInPlane() const { return satNumberInPlane; }

    std::pair<cGate *, ISLState> getInputGate(isldirection::ISLDirection direction, int index = -1);
    std::pair<cGate *, ISLState> getOutputGate(isldirection::ISLDirection direction, int index = -1);
    std::pair<cGate *, ISLState> getGate(isldirection::ISLDirection direction, cGate::Type type, int index);
    /** @brief Connects the satellite to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    bool connect(Sat *other, isldirection::ISLDirection direction);

    /** @brief Disconnects the satellite on the given direction. The return value indicates whether the connection was deleted or did not exist.*/
    bool disconnect(isldirection::ISLDirection direction);
    //  /** @brief Connects the satellite via right Inter-Plane ISL to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    //  bool connectRight(SatelliteRoutingBase *other);

    bool hasLeftSat() const;
    bool hasUpSat() const;
    bool hasRightSat() const;
    bool hasDownSat() const;

    Sat *getLeftSat() const;
    Sat *getUpSat() const;
    Sat *getRightSat() const;
    Sat *getDownSat() const;

    void setLeftSat(Sat *newSat);
    void setUpSat(Sat *newSat);
    void setRightSat(Sat *newSat);
    void setDownSat(Sat *newSat);

    void removeLeftSat();
    void removeUpSat();
    void removeRightSat();
    void removeDownSat();

    int getLeftSatId() const;
    int getUpSatId() const;
    int getRightSatId() const;
    int getDownSatId() const;

    double getLeftSatDistance() const;
    double getUpSatDistance() const;
    double getRightSatDistance() const;
    double getDownSatDistance() const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    ISLState getLeftSendState() const { return leftSendState; }
    ISLState getLeftRecvState() const { return leftRecvState; }
    ISLState getUpSendState() const { return upSendState; }
    ISLState getUpRecvState() const { return upRecvState; }
    ISLState getRightSendState() const { return rightSendState; }
    ISLState getRightRecvState() const { return rightRecvState; }
    ISLState getDownSendState() const { return downSendState; }
    ISLState getDownRecvState() const { return downRecvState; }

    void setLeftSendState(ISLState newState);
    void setLeftRecvState(ISLState newState);
    void setUpSendState(ISLState newState);
    void setUpRecvState(ISLState newState);
    void setRightSendState(ISLState newState);
    void setRightRecvState(ISLState newState);
    void setDownSendState(ISLState newState);
    void setDownRecvState(ISLState newState);

    void setISLSendState(isldirection::ISLDirection direction, ISLState state);
    void setISLRecvState(isldirection::ISLDirection direction, ISLState state);

    /** @brief Returns number of planes. */
    double getNumberOfPlanes() const;
    /** @brief Returns number of sats per plane. */
    double getSatsPerPlane() const;
    /** @brief Returns the mean anomaly of this entity. */
    double getMnAnomaly() const;
    /** @brief Returns the RAAN of this entity. */
    double getRAAN() const;
    /** @brief Returns the elevation from this entity to a reference entity. */
    double getElevation(const PositionAwareBase &other);
    /** @brief Returns the elevation from this entity to a reference entity with offset into future. */
    double getFutureElevation(const PositionAwareBase &other, simtime_t offset) const;
    /** @brief Returns the azimuth from this entity to a reference entity. */
    double getAzimuth(const PositionAwareBase &other) const;
    /** @brief Returns whether the satellite is currently ascending. */
    bool isAscending() const;
    /** @brief Returns whether the satellite is currently descending. */
    bool isDescending() const;

    /** @brief Returns whether the satellite is currently able to create inter plane ISL connections. */
    bool isInterPlaneISLEnabled() const;

    friend std::ostream &operator<<(std::ostream &ss, const Sat &p) {
        ss << "{";
        ss << "\"satelliteId\": " << p.satId << ",";
        ss << "\"up\": " << p.upSatellite << ",";
        ss << "\"down\": " << p.downSatellite << ",";
        ss << "\"left\": " << p.leftSatellite << ",";
        ss << "\"right\": " << p.rightSatellite << ",";
        ss << "}";
        return ss;
    }

   protected:
    virtual void finish() override;
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    void setISLState(isldirection::ISLDirection direction, bool send, ISLState state);

    void handleMessage(cMessage *msg) override;

   public:
    void printConnectedSatellites();



#ifndef NDEBUG
    virtual void refreshDisplay() const override;
#endif
};

}  // namespace satellite

#endif /* NODES__20_SATELLITE_SAT_H_ */

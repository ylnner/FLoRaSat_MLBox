/*
 * DtSIoTPropagation.h
 *
 *  Created on: May 15, 2022
 *      Author: diego
 */

#ifndef LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_
#define LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_

#include <math.h>

#include "../../../Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/PropagationBase.h"
#include "inet/physicallayer/wireless/common/signal/Arrival.h"

#include "Global/Channel/Unitdisk/SatelliteUnitDiskTransmission.h"
#include "Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.h"
#include "Global/Transceiver/LoRaTransmission.h"

#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Utilities/libnorad/cEci.h"
#include "Global/Utilities/libnorad/ccoord.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"


using namespace mobility;

namespace propagation {

/**Class: DtSIoTPropagation
 * Within this model the distance between two positions are calculated using the coordinates of the source and destination.
 * This distanced is used to calculate the propagation delay for nodes within a satellite constellation.
 * Written by Aiden Valentine
 * Modified by Diego Maldonado
 */
class DtSIoTPropagation : public PropagationBase {
   protected:
    bool ignoreMovementDuringTransmission;
    bool ignoreMovementDuringPropagation;
    bool ignoreMovementDuringReception;

   protected:
    virtual void initialize(int stage) override;
    virtual const Coord computeArrivalPosition(const simtime_t startTime, const Coord startPosition, IMobility *mobility) const;

   public:
    DtSIoTPropagation();

    virtual std::ostream &printToStream(std::ostream &stream, int level, int evFlags = 0) const override;
    virtual const IArrival *computeArrival(const ITransmission *transmission, IMobility *mobility) const override;
};

}  // namespace flora

#endif /* LORAPHY_PROPAGATION_DTSIOTPROPAGATION_H_ */

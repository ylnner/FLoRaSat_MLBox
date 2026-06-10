//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef CUSTOMPHYDMODEL_H_
#define CUSTOMPHYANALOGMODEL_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/ScalarAnalogModelBase.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
#include "Global/Channel/CustomBandListening.h"
#include "Global/Utilities/CSVReader.h"
#include "../../Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"

using namespace utilities;

namespace channel {

/**
 * Uses fixed frequency 401.63 MHz for satellite communications
 */
class CustomPHYAnalogModel : public ScalarAnalogModelBase
{
protected:
    
    // Antenna pattern for transmitter gain computation
    AntennaPattern transmitterAntennaPattern;

    
    // Statistics counters
    double totalReceivedPower_dBm;
    int packetReceivedCount;
    
    // Vector recorder for received power
    cOutVector *expectedReceivedPowerVector;
    cOutVector *pathLossVector;
    cOutVector *txAntennaGainVector;
    cOutVector *rxAntennaGainVector;

protected:
    virtual void initialize(int stage) override;
    virtual void finish() override;
    
    /**
        * Compute antenna gain from spherical angles (radians)
     */
    double computeAntennaGain(double theta_rad, double phi_rad) const;

public:
    
    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
    virtual W computeReceptionPower(const IRadio *radio, const ITransmission *transmission, const IArrival *arrival) const override;
    virtual const IReception *computeReception(const IRadio *radio, const ITransmission *transmission, const IArrival *arrival) const override;
};

} // namespace channel

#endif /* CUSTOMPHYANALOGMODEL_H_ */

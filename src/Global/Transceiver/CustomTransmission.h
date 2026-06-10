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

#ifndef CUSTOMTRANSMISSION_H_
#define CUSTOMTRANSMISSION_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/TransmissionBase.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioSignal.h"
#include "Global/Utilities/libnorad/ccoord.h"

namespace inet {
namespace physicallayer {

/**
 * Custom transmission class for CSV-based PHY layer
 * Removes LoRa-specific parameters (SF, BW, CR)
 * Uses fixed frequency of 401.63 MHz for satellite communications
 */
class CustomTransmission : public TransmissionBase, public virtual INarrowbandSignal, public virtual IScalarSignal
{
protected:
    const W transmissionPower;          // Transmission power
    const Hz centerFrequency;           // Center frequency (401.63 MHz)
    const Hz bandwidth;                 // Bandwidth
    cCoordGeo longLatStartPosition;     // Geographic position at start
    cCoordGeo longLatEndPosition;       // Geographic position at end
    
public:
    CustomTransmission(
        const IRadio *transmitter, 
        const Packet *macFrame, 
        const simtime_t startTime, 
        const simtime_t endTime, 
        const simtime_t preambleDuration, 
        const simtime_t headerDuration, 
        const simtime_t dataDuration, 
        const Coord startPosition, 
        const Coord endPosition, 
        const Quaternion startOrientation, 
        const Quaternion endOrientation, 
        W transmissionPower, 
        Hz centerFrequency, 
        Hz bandwidth,
        cCoordGeo longLatStartPosition, 
        cCoordGeo longLatEndPosition);

    virtual Hz getCenterFrequency() const override { return centerFrequency; }
    virtual Hz getBandwidth() const override { return bandwidth; }
    virtual W getPower() const override { return transmissionPower; }
    virtual W computeMinPower(const simtime_t startTime, const simtime_t endTime) const override { return transmissionPower; }

    W getTransmissionPower() const { return transmissionPower; }
    
    virtual cCoordGeo getStartLongLatPosition() const { return longLatStartPosition; }
    virtual cCoordGeo getEndLongLatPosition() const { return longLatEndPosition; }

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
};

} /* namespace physicallayer */
} /* namespace inet */

#endif /* CUSTOMTRANSMISSION_H_ */

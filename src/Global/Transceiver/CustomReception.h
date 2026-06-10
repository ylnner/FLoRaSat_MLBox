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

#ifndef CUSTOMRECEPTION_H_
#define CUSTOMRECEPTION_H_

#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarReception.h"

using namespace inet;
using namespace inet::physicallayer;

namespace transceiver {

/**
 * Custom reception class for CSV-based PHY layer
 * Removes LoRa-specific parameters (SF, BW, CR)
 */
class CustomReception : public ScalarReception
{
protected:
    const Hz centerFrequency;           // Center frequency
    const Hz bandwidth;                 // Bandwidth
    const W receivedPower;              // Received power
    
public:
    CustomReception(
        const IRadio *radio, 
        const ITransmission *transmission, 
        const simtime_t startTime, 
        const simtime_t endTime, 
        const Coord startPosition, 
        const Coord endPosition, 
        const Quaternion startOrientation, 
        const Quaternion endOrientation, 
        Hz centerFrequency, 
        Hz bandwidth, 
        W receivedPower);

    Hz getCenterFrequency() const { return centerFrequency; }
    Hz getBandwidth() const { return bandwidth; }

    virtual W getPower() const override { return receivedPower; }
    virtual W computeMinPower(simtime_t startTime, simtime_t endTime) const override;
};

} // namespace transceiver

#endif /* CUSTOMRECEPTION_H_ */

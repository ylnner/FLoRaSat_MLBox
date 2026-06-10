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

#ifndef CUSTOMBANDLISTENING_H_
#define CUSTOMBANDLISTENING_H_

#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"

using namespace inet;
using namespace inet::physicallayer;

namespace channel {

/**
 * Custom band listening class for CSV-based PHY layer
 */
class CustomBandListening : public BandListening
{
protected:
    Hz centerFrequency;
    Hz bandwidth;

public:
    CustomBandListening(
        const IRadio *radio,
        const simtime_t startTime,
        const simtime_t endTime,
        const Coord startPosition,
        const Coord endPosition,
        Hz centerFrequency,
        Hz bandwidth);

    Hz getCenterFrequency() const { return centerFrequency; }
    Hz getBandwidth() const { return bandwidth; }
};

} // namespace channel

#endif /* CUSTOMBANDLISTENING_H_ */

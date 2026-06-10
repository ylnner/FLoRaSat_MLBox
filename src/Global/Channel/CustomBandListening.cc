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

#include "CustomBandListening.h"

namespace channel {

CustomBandListening::CustomBandListening(
    const IRadio *radio,
    const simtime_t startTime,
    const simtime_t endTime,
    const Coord startPosition,
    const Coord endPosition,
    Hz centerFrequency,
    Hz bandwidth) :
        BandListening(radio, startTime, endTime, startPosition, endPosition, centerFrequency, bandwidth),
        centerFrequency(centerFrequency),
        bandwidth(bandwidth)
{
}

} // namespace channel

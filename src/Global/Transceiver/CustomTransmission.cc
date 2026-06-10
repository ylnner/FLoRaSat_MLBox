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

#include "CustomTransmission.h"

namespace inet {
namespace physicallayer {

CustomTransmission::CustomTransmission(
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
    cCoordGeo longLatEndPosition) :
        TransmissionBase(transmitter, macFrame, startTime, endTime, preambleDuration, headerDuration, dataDuration, startPosition, endPosition, startOrientation, endOrientation),
        transmissionPower(transmissionPower),
        centerFrequency(centerFrequency),
        bandwidth(bandwidth)
{
    this->longLatStartPosition = longLatStartPosition;
    this->longLatEndPosition = longLatEndPosition;
}

std::ostream& CustomTransmission::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "CustomTransmission, power=" << transmissionPower 
           << ", freq=" << centerFrequency 
           << ", bw=" << bandwidth;
    return TransmissionBase::printToStream(stream, level);
}

} /* namespace physicallayer */
} /* namespace inet */

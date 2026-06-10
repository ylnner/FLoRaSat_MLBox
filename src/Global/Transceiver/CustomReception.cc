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

#include "CustomReception.h"

namespace transceiver {

CustomReception::CustomReception(
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
    W receivedPower) :
        ScalarReception(radio, transmission, startTime, endTime, startPosition, endPosition, startOrientation, endOrientation, centerFrequency, bandwidth, receivedPower),
        centerFrequency(centerFrequency),
        bandwidth(bandwidth),
        receivedPower(receivedPower)
{
}

W CustomReception::computeMinPower(simtime_t startTime, simtime_t endTime) const
{
    return receivedPower;
}

} // namespace transceiver

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

package Global.Transceiver;

import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandTransmitterBase;

//
// Custom transmitter for CSV-based PHY layer
// Removes LoRa-specific parameters and uses fixed frequency 401.63 MHz
//
module CustomTransmitter extends NarrowbandTransmitterBase
{
    parameters:
        @class(transceiver::CustomTransmitter);
        @signal[transmissionCreated];
        headerLength = default(0b);
        bitrate = default(100bps);
        power;
        centerFrequency = default(401.63MHz);  // Fixed for satellite communications
        bandwidth = default(125kHz);
        modulation = default("LoRaModulation");
}

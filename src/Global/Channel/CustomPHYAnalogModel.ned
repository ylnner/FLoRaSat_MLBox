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

package Global.Channel;

import inet.physicallayer.wireless.common.contract.packetlevel.IAnalogModel;

//
// Custom PHY analog model with link budget computation and CSV-based noise
// Removes LoRa-specific hardcoded noise tables
//
module CustomPHYAnalogModel like IAnalogModel
{
    parameters:
        @class(channel::CustomPHYAnalogModel);
        bool ignorePartialInterference = default(false);
        string antennaPatternFile = default("");  // Path to antenna pattern CSV file
}

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

package Global.Antenna;

import inet.physicallayer.wireless.common.base.packetlevel.AntennaBase;

//
// Custom antenna that loads 3D radiation patterns from CSV files
// CSV format: Phi (azimuth 0-360°), Theta (elevation 0-180°), Gain (dBi)
//
module CustomAntenna extends AntennaBase
{
    parameters:
        @class(antenna::CustomAntenna);
        string antennaPatternFile = default("");  // Path to antenna pattern CSV file
}

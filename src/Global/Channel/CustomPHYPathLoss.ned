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

import inet.physicallayer.wireless.common.pathloss.FreeSpacePathLoss;

//
// Path loss model for satellite communications.
// Extends free space path loss with additional losses:
// - Polarization loss (circular-linear mismatch)
// - Log-normal fading
// - Receiver system loss (cables, connectors, etc.)
//
module CustomPHYPathLoss extends FreeSpacePathLoss
{
    parameters:
        // Polarization loss in dB (typically 3 dB for circular-linear mismatch)
        double polarizationLoss @unit(dB) = default(0dB);
        
        // Log-normal fading standard deviation in dB
        double fadingSigma @unit(dB) = default(0dB);
        
        // Receiver system loss in dB (cable losses, connector losses, etc.)
        double receiverSystemLoss @unit(dB) = default(0dB);

        double decodingImplementation @unit(dB) = default(0dB);

        double noiseFigureEnhancement @unit(dB) = default(0dB);
        
        @class(channel::CustomPHYPathLoss);
}

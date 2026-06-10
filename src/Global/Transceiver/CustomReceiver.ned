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

import inet.physicallayer.wireless.common.contract.packetlevel.IReceiver;

//
// Custom receiver for CSV-based PHY layer with C/N0-based decision logic
// Removes LoRa-specific collision detection and uses CSV-loaded C/N0 mappings
//
module CustomReceiver like IReceiver
{
    parameters:
        @class(transceiver::CustomReceiver);
        @signal[receptionCollision](type=bool);
        @signal[belowSensitivity](type=long);
        @statistic[numCollisions](title="Number of collisions"; source=count(receptionCollision); record=last);
        @statistic[numCN0Failures](title="Number of C/N0-based reception failures"; record=last);
        
		double energyDetection @unit(dBm);  // no signal is detected at all below this reception power threshold (idle state)
        double sensitivity @unit(dBm);      // reception is not possible if the signal power is below sensitivity (idle or busy states)
        double snirThreshold @unit(dB);     // reception is not successful if the SNIR is below this threshold (unsuccessful reception)

        
        // Custom PHY parameters
        string cn0MapFile = default("");  // Path to C/N0-to-reception-rate CSV file
        double noiseFloor_dBm = default(-120);  // Noise floor in dBm
        double centerFrequency @unit(Hz);  // Center frequency for reception
        double bandwidth @unit(Hz);  // Bandwidth for reception
        
        string relativeInterferenceMapFile = default(""); // Path to relative interference map CSV file
}

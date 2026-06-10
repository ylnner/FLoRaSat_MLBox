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
import inet.physicallayer.wireless.common.contract.packetlevel.IErrorModel;

//
// TODO auto-generated module
//
module LoRaReceiver like IReceiver
{
        parameters:
        @signal[LoRaReceptionCollision](type=bool); // optional
        @signal[belowSensitivityReception](type=int); // optional

        @signal[LoRa_Receiver_TranmissionStatistics](type=cMessage);//optional

        @statistic[LoRaReceptionCollision](source=LoRaReceptionCollision; record=vector); // record=count
        double energyDetection @unit(dBm);  // no signal is detected at all below this reception power threshold (idle state)
        double sensitivity @unit(dBm);      // reception is not possible if the signal power is below sensitivity (idle or busy states)
        double snirThreshold @unit(dB);     // reception is not successful if the SNIR is below this threshold (unsuccessful reception)
        double centerFrequency @unit(Hz);   // center frequency of the band where this receiver listens on the medium
        double bandwidth @unit(Hz);         // bandwidth of the band where this receiver listens on the medium
        bool alohaChannelModel = default(false);
        string errorModelType = default("");             // NED type of the error model
        bool ignoreInterference = default(false);
        @class(transceiver::LoRaReceiver);
        @display("i=block/wrx");

        // ACHF
		//bool enableCollisions = default(true);             // If false, the receiver ignores all interference and only considers the desired signal's power for reception success
        bool enableCollisionOverlap = default(false);       // If true, the receiver considers temporal overlap of receptions for collision detection
        double minCollisionOverlap @unit(s) = default(0s);  // Minimum temporal overlap to count as a collision

    submodules:
        errorModel: <errorModelType> like IErrorModel if errorModelType != "";
}

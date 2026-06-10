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

package Nodes._10_Terminal._10_Physical;

import Global.Transceiver.CustomReceiver;
import Global.Transceiver.CustomTransmitter;
import inet.physicallayer.wireless.common.base.packetlevel.FlatRadioBase;
import inet.power.storage.IdealEpEnergyStorage;

//
// Custom radio for terminals using CSV-based PHY layer
// Uses CustomTransmitter and CustomReceiver with CustomMedium
//
module Ter_Phy_CustomRadio extends FlatRadioBase
{
    parameters:
        @networkNode();
        @signal[packetSentToUpper](type=cPacket);
        @signal[packetReceivedFromUpper](type=cPacket);
        @signal[radioModeChanged](type=long);
        @signal[listeningChanged];
        @signal[receptionStateChanged](type=long);
        @signal[transmissionStateChanged](type=long);
        @signal[receivedSignalPartChanged](type=long);
        @signal[transmittedSignalPartChanged](type=long);
        @signal[transmissionStarted];
        @signal[transmissionEnded];
        @signal[receptionStarted];
        @signal[receptionEnded];
        @signal[minSNIR];
        @signal[packetErrorRate];
        @signal[bitErrorRate];
        @signal[symbolErrorRate];
        @signal[droppedPacket](type=long);

        @statistic[radioMode](title="Radio mode"; source=radioModeChanged; record=count,vector; interpolationmode=sample-hold);
        @statistic[receptionState](title="Radio reception state"; source=receptionStateChanged; record=count,vector; interpolationmode=sample-hold);
        @statistic[transmissionState](title="Radio transmission state"; source=transmissionStateChanged; record=count,vector; interpolationmode=sample-hold);

        antenna.typename = default("CustomAntenna");
        transmitter.typename = default("CustomTransmitter");
        receiver.typename = default("CustomReceiver");

        centerFrequency = 401.63 MHz;  // VHF satellite frequency
        bandwidth = default(0.25 MHz);

        *.bitrate = default(250 kbps);

        // PHY Header
        *.headerBitLength = (4*8 + 1*8 + 7 + 1) * 1 b;
        transmitter.headerLength = 5 B;

        // Receiver sensitivity for custom PHY
        receiver.energyDetection = default(-90dBm);
        receiver.sensitivity = default(-137dBm);
        receiver.minInterferencePower = default(-120dBm);
        receiver.snirThreshold = default(-8 dB);

        // Default transmitter power
        transmitter.power = default(500mW);
        transmitter.preambleDuration = 0.001s;

        bool iAmGateway = default(false);

        @class(radio::Ter_Phy_CustomRadio);
        @display("bgb=200,575");

    submodules:
        IdealEpEnergyStorage: IdealEpEnergyStorage {
            @display("p=100,500");
        }
}

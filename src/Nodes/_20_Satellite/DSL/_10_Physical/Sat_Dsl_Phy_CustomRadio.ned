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

package Nodes._20_Satellite.DSL._10_Physical;

import inet.physicallayer.wireless.common.base.packetlevel.FlatRadioBase;
import inet.power.storage.IdealEpEnergyStorage;

//
// Custom radio for satellites using CSV-based PHY layer
// Supports concurrent transmissions and receptions
//
module Sat_Dsl_Phy_CustomRadio extends FlatRadioBase
{
    parameters:
        @class(radio::Sat_Dsl_Phy_CustomRadio);
        
        bool iAmGateway = default(true);
        int satIndex = default(0);

        antenna.typename = default("CustomAntenna");
        transmitter.typename = default("CustomTransmitter");
        receiver.typename = default("CustomReceiver");

        @signal[Sat_Dsl_Phy_CustomRadioReceptionStarted];
        @signal[Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect];
        @signal[Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring];
        
        @statistic[Sat_Dsl_Phy_CustomRadioReceptionStarted](source=Sat_Dsl_Phy_CustomRadioReceptionStarted; record=vector);
        @statistic[Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect](source=Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect; record=vector);
        @statistic[Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring](source=Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring; record=vector);

        centerFrequency = 401.63 MHz;  // VHF satellite frequency
        bandwidth = default(0.25 MHz);

        *.bitrate = default(250 kbps);
        *.headerBitLength = (4*8 + 1*8 + 7 + 1) * 1 b;
        transmitter.headerLength = 5 B;

        receiver.energyDetection = default(-90dBm);
        receiver.sensitivity = default(-137dBm);
        receiver.minInterferencePower = default(-120dBm);
        receiver.snirThreshold = default(-8 dB);
        transmitter.power = default(27mW);
        transmitter.preambleDuration = 0.001s;

        @display("bgb=200,575");

    submodules:
        IdealEpEnergyStorage: IdealEpEnergyStorage {
            @display("p=100,500");
        }
}

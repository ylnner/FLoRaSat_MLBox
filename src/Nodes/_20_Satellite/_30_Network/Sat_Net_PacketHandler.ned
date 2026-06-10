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

package Nodes._20_Satellite._30_Network;

//
// Central packet handler that receives all incoming packets and coordinates routing.
// 
// This module serves as the main entry point for all packets arriving at the satellite
// network layer from ISL, GSL, and DSL interfaces. Upon receiving a packet, it forwards
// it to the routing module for a routing decision. The routing module then calls back
// sendMessage() to forward the packet to the appropriate output gate.
//
simple Sat_Net_PacketHandler {
    parameters:
        @class(satellite::Sat_Net_PacketHandler);
        @display("i=block/switch");
        
        int numGroundLinks = default(60);
        
        @signal[packetReceived](type=inet::Packet);
        @signal[packetSent](type=inet::Packet);
        @signal[packetDropped](type=inet::Packet);
        
        @statistic[packetsReceived](source=packetReceived; record=count,vector);
        @statistic[packetsSent](source=packetSent; record=count,vector);
        @statistic[packetsDropped](source=packetDropped; record=count,vector);
    
    gates:
        // ISL gates
        input leftIn;
        output leftOut;
        input upIn;
        output upOut;
        input rightIn;
        output rightOut;
        input downIn;
        output downOut;
        
        // Ground Station gates
        input groundLinkIn[numGroundLinks];
        output groundLinkOut[numGroundLinks];
        
        // Device/Terminal LoRa gates
        input lowerLayerLoRaIn @labels(LoRaGate/up);
        output lowerLayerLoRaOut @labels(LoRaGate/down);
        
        input lowerLayerGWLoRaIn @labels(LoRaGate/up);
        output lowerLayerGWLoRaOut @labels(LoRaGate/down);
        
        // Lower layer connections (from parent compound module)
        input lowerInDSL;
        output lowerOutDSL;
        input lowerInISL;
        output lowerOutISL;
        input lowerInGSL;
        output lowerOutGSL;
}
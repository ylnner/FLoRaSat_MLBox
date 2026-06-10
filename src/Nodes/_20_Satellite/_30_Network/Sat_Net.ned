package Nodes._20_Satellite._30_Network;

import Nodes._20_Satellite._30_Network.Sat_Net_PacketHandler;
///import inet.queueing.queue.DropTailQueue;
///import Global.Base._30_Network.packetprocessing.TagBasedPacketDelayer;

//
// Satellite Network Layer - Compound module coordinating routing and packet forwarding.
//
// Architecture:
// - @class annotation attaches Sat_Net_SatelliteRouting for statistics collection
// - packetHandler: Central dispatcher receiving all packets from all gates
// - routing: Pluggable routing algorithm (selected via routingType parameter)
//
// Flow:
// 1. Packets arrive at input gates → routed to packetHandler via NED connections
// 2. PacketHandler forwards to routing module via handlePacket()
// 3. Routing module calls back sender->sendMessage() with routing decision
// 4. PacketHandler executes the decision (maps ISLDirection to physical gates)
// 5. SatelliteRouting (@class) collects statistics from emitted signals
//
// To add custom routing: Create module extending Base_Routing, set routingType parameter
//
module Sat_Net
{
    parameters:
        @class(satellite::Sat_Net_SatelliteRouting);
        @display("i=block/network");
        
        int numGroundLinks = default(60);
        
        // Routing algorithm selection
        string routingType = default("StoreAndForward");

    gates:
        // ISL gates - connected to ISL data link layer
        input leftIn;
        output leftOut;
        input upIn;
        output upOut;
        input rightIn;
        output rightOut;
        input downIn;
        output downOut;

        // Ground Station gates - connected to GSL data link layer
        input groundLinkIn[numGroundLinks];
        output groundLinkOut[numGroundLinks];

        // Device/Terminal gates - connected to DSL data link layer
        input lowerLayerLoRaIn @labels(LoRaGate/up);
        output lowerLayerLoRaOut @labels(LoRaGate/down);

        input lowerLayerGWLoRaIn @labels(LoRaGate/up);
        output lowerLayerGWLoRaOut @labels(LoRaGate/down);

        // Connections to lower layers (from parent compound module)
        input lowerInDSL;
        output lowerOutDSL;
        input lowerInISL;
        output lowerOutISL;
        input lowerInGSL;
        output lowerOutGSL;

    submodules:
        // Packet handler - forwards packets to correct gates based on routing decisions
        packetHandler: Sat_Net_PacketHandler {
            parameters:
                numGroundLinks = parent.numGroundLinks;
                @display("p=170,120");
        }

        // Routing algorithm - makes routing decisions (e.g., store-and-forward)
        routing: <routingType> like Global.Base._30_Network.Base_IRouting {
            @display("p=170,50");
        }

    connections allowunconnected:
        // ISL connections - passthrough to packet handler
        leftIn --> packetHandler.leftIn;
        packetHandler.leftOut --> leftOut;
		
		
		
        upIn --> packetHandler.upIn;
		packetHandler.upOut --> upOut;

        rightIn --> packetHandler.rightIn;
        packetHandler.rightOut --> rightOut;

        downIn --> packetHandler.downIn;
        packetHandler.downOut --> downOut;

        // Ground link connections - passthrough to packet handler
        for i=0..numGroundLinks-1 {
            groundLinkIn[i] --> packetHandler.groundLinkIn[i];
            packetHandler.groundLinkOut[i] --> groundLinkOut[i];
        }

        // LoRa/Device connections - passthrough to packet handler
        lowerLayerLoRaIn --> packetHandler.lowerLayerLoRaIn;
        packetHandler.lowerLayerLoRaOut --> lowerLayerLoRaOut;

        lowerLayerGWLoRaIn --> packetHandler.lowerLayerGWLoRaIn;
        packetHandler.lowerLayerGWLoRaOut --> lowerLayerGWLoRaOut;

        // Lower layer connections
        lowerInDSL --> packetHandler.lowerInDSL;
        packetHandler.lowerOutDSL --> lowerOutDSL;

        lowerInISL --> packetHandler.lowerInISL;
        packetHandler.lowerOutISL --> lowerOutISL;

        lowerInGSL --> packetHandler.lowerInGSL;
        packetHandler.lowerOutGSL --> lowerOutGSL;
        
        
        
        
}

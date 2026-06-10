package Nodes._30_Station._30_Network;

import Global.Utilities.LayerForwarder;
import Nodes._30_Station._60_Mobility.Sta_Mob_StationMobility;
import inet.node.contract.INetworkNode;

//module Sta_Net extends LayerForwarder
module Sta_Net extends LayerForwarder like INetworkNode
{
    parameters:
        layerName = default("StationNetworkLayer");
        @display("i=block/network");
        
        @class(Sta_Net);
        //@display("i=device/receiverdish");
        int numGroundLinks 		 = default(60);
        bool hasIpv4 			 = default(true);
        int numUdpApps 			 = default(0);
        bool hasUdp 			 = default(firstAvailableOrEmpty("Udp") != "");
        string udpType 			 = default(firstAvailableOrEmpty("UDP"));
        string networkLayerType  = default("Ipv4NetworkLayer");
        string routingTsableType = default("Ipv4RoutingTable");
        bool sendData @mutable 	 = default(false);
        int dstGsOrDev 			 = default(1);
	gates:
		inout ethg @labels(EtherFrame-conn) @allowUnconnected;
        inout satelliteLink[numGroundLinks] @labels(EtherFrame-conn);
           
}

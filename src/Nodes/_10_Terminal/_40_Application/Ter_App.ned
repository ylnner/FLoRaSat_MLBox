package Nodes._10_Terminal._40_Application;


import Global.Utilities.LayerForwarder;
//import Nodes._10_Terminal._40_Application.MLBox.Ter_App_MLBox;
module Ter_App extends LayerForwarder
{
    parameters:
        layerName = default("TerminalApplicationLayer");
        @display("i=block/app");
        @class(Ter_App);

        //@signal[LoRa_AppPacketSent](type=long); // optional
        //@statistic[LoRa_AppPacketSent](source=LoRa_AppPacketSent; record=count);
        int numberOfPacketsToSend 				   = default(0);
        volatile double timeToFirstPacket @unit(s) = default(10s);
        volatile double timeToNextPacket @unit(s)  = default(10s);
        volatile double timeToResponse @unit(s)    = default(30s);
        volatile double timeToRetry @unit(s)       = default(10s);
        volatile double ackTimeout @unit(s) 	   = default(7s);
        volatile double timer @unit(s) 			   = default(1s);
        double initialLoRaTP @unit(dBm) 		   = default(14dBm);
        double initialLoRaCF @unit(Hz) 			   = default(868MHz);
        int initialLoRaSF 						   = default(12);
        double initialLoRaBW @unit(Hz) 			   = default(125kHz);
        int initialLoRaCR 					       = default(4);
        bool initialUseHeader 					   = default(true);
        int initialPingSlot 					   = default(1);
        bool InitClassA 						   = default(true);
        bool InitClassB 						   = default(false);
        bool evaluateADRinNode 					   = default(false);
        int payloadSize @unit(B) 				   = default(10B);
        bool usingAck 							   = default(true);


        // ACHF
        int numGroundLinks 						   = default(60);
        int maxHops 	   						   = default(50);
        int nroMaxAttempts						   = default(0);

     gates:
        output appOut @labels(LoRaAppPacket/down);
        input appIn @labels(LoRaAppPacket/up);
	
	//submodules:
	//	mlBox: Ter_App_MLBox {
    //        @display("p=100,250");
    //    }
}

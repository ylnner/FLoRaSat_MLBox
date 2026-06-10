package Nodes._20_Satellite.DSL._20_Data_Link;

import Nodes._20_Satellite.DSL._20_Data_Link.Sat_Dsl_Dlk_kMac;
import inet.linklayer.contract.IMacProtocol;

module Sat_Dsl_Dlk
{
    parameters:
        @display("i=block/layer");
        
    gates:
        input upperIn @optional;
        output upperOut @optional;
        input lowerIn @optional;
        output lowerOut @optional;
        
    submodules:
        mac: <default("Sat_Dsl_Dlk_kMac")> like IMacProtocol {
            @display("p=100,100");
        }
        
    connections:
        upperIn --> mac.upperLayerIn;
        mac.upperLayerOut --> upperOut;
        lowerIn --> mac.lowerLayerIn;
        mac.lowerLayerOut --> lowerOut;
}
	
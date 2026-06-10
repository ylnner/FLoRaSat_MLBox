package Nodes._10_Terminal._20_Data_Link;

import Global.Utilities.LayerForwarder;
import Nodes._10_Terminal._20_Data_Link.Ter_Dlk_KiWan;
import inet.linklayer.contract.IMacProtocol;


module Ter_Dlk
{
    parameters:
        @display("i=block/layer");
        
    gates:
        input upperIn @optional;
        output upperOut @optional;
        input lowerIn @optional;
        output lowerOut @optional;
        
    submodules:
        mac: <default("Ter_Dlk_KiWan")> like IMacProtocol {
            @display("p=100,100");
        }
        
    connections allowunconnected:
        upperIn --> mac.upperLayerIn;
        mac.upperLayerOut --> upperOut;
        
        lowerIn --> mac.lowerLayerIn;
        mac.lowerLayerOut --> lowerOut;
}

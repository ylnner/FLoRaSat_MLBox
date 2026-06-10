package Nodes._30_Station._20_Data_Link;

import Nodes._30_Station._20_Data_Link.Sta_Dlk_Statistics;

module Sta_Dlk
{
    parameters:
        @display("i=block/layer");
    
    gates:
        input upperIn @optional;
        output upperOut @optional;
        input lowerIn @optional;
        output lowerOut @optional;
        
        
	submodules:
        mac: Sta_Dlk_Statistics {
            @display("p=100,100");
        }
                
        
    connections allowunconnected:
        upperIn --> mac.upperLayerIn;
        mac.upperLayerOut --> upperOut;
        
        lowerIn --> mac.lowerLayerIn;
        mac.lowerLayerOut --> lowerOut;
}

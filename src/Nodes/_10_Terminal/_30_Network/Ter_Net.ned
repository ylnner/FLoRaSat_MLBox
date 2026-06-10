// It's only a simple network layer for terminal nodes 
// that forwards packets to upper and lower layers.

package Nodes._10_Terminal._30_Network;

import Global.Utilities.LayerForwarder;

module Ter_Net extends LayerForwarder
{
    parameters:
        layerName = default("Ter_Net");
        @display("i=block/network");
        
        @class(terminal::Ter_Net_TerminalRouting);
        string routingType;
	submodules:
		// Routing algorithm - makes routing decisions (e.g., store-and-forward)
        routing: <routingType> like Global.Base._30_Network.Ter_Base_IRouting {
            @display("p=170,50");
        }

}

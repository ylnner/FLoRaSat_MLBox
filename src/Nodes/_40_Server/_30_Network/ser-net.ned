package Nodes._40_Server._30_Network;

import Global.Utilities.LayerForwarder;

simple ServerNetworkLayer extends LayerForwarder
{
    parameters:
        layerName = default("ServerNetworkLayer");
        @display("i=block/network");
}

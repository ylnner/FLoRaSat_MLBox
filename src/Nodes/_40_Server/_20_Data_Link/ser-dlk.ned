package Nodes._40_Server._20_Data_Link;

import Global.Utilities.LayerForwarder;

simple ServerDataLinkLayer extends LayerForwarder
{
    parameters:
        layerName = default("ServerDataLinkLayer");
        @display("i=block/layer");
}

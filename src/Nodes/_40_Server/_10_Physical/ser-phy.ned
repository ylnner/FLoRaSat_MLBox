package Nodes._40_Server._10_Physical;

import Global.Utilities.LayerForwarder;

simple ServerPhysicalLayer extends LayerForwarder
{
    parameters:
        layerName = default("ServerPhysicalLayer");
        @display("i=block/tunnel");
}

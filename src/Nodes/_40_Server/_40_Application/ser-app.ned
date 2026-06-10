package Nodes._40_Server._40_Application;

import Global.Utilities.LayerForwarder;

simple ServerApplicationLayer extends LayerForwarder
{
    parameters:
        layerName = default("ServerApplicationLayer");
        @display("i=block/app");
}

package Nodes._30_Station._40_Application;

import Global.Utilities.LayerForwarder;

simple StationApplicationLayer extends LayerForwarder
{
    parameters:
        layerName = default("StationApplicationLayer");
        @display("i=block/app");
}

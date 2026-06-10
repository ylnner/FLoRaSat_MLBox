package Nodes._20_Satellite._40_Application;

import Global.Utilities.LayerForwarder;

simple Sat_App extends LayerForwarder
{
    parameters:
        layerName = default("Sat_App");
        @display("i=block/app");
}

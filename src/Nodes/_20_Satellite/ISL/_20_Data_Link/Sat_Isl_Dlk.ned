package Nodes._20_Satellite.ISL._20_Data_Link;

import Global.Utilities.LayerForwarder;

simple Sat_Isl_Dlk extends LayerForwarder
{
    parameters:
        layerName = default("Sat_Isl_Dlk");
        @display("i=block/layer");
}
	
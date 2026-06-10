package Nodes._20_Satellite.GSL._10_Physical;

import Global.Utilities.LayerForwarder;

//simple 
module Sat_Gsl_Phy extends LayerForwarder
{
    parameters:
        @networkNode();
        layerName = default("Sat_Gsl_Phy");
        @display("i=block/tunnel");
        
        int numGroundLinks = default(60);
        
	gates:
		input groundLinkIn[numGroundLinks];
        output groundLinkOut[numGroundLinks];
	
	connections allowunconnected: 
}

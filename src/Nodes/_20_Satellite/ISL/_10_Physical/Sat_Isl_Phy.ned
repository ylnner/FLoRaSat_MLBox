package Nodes._20_Satellite.ISL._10_Physical;

import Global.Utilities.LayerForwarder;
import Nodes._20_Satellite._60_Mobility.Sat_Mob_SatelliteMobility;

module Sat_Isl_Phy extends LayerForwarder
{
    parameters:
        @networkNode();
        layerName = default("Sat_Isl_Phy");
        @display("i=block/tunnel");
                
        double upperLatitudeBound = default(70.0);
        double lowerLatitudeBound = default(-70.0);
	
	gates:
		// ISL gates
        input leftIn;
        output leftOut;
        input upIn;
        output upOut;
        input rightIn;
        output rightOut;
        input downIn;
        output downOut;    
        

	submodules: 
		leftIf: ISLQueueInterface;
        upIf: ISLQueueInterface;
        rightIf: ISLQueueInterface;
        downIf: ISLQueueInterface;
        //glIf: GLQueueInterface;
        
    connections allowunconnected:
        leftIf.out --> leftOut;
        upIf.out --> upOut;
        rightIf.out --> rightOut;
        downIf.out --> downOut;
    
}

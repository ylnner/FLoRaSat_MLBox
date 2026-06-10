package Nodes._20_Satellite._60_Mobility;

//import inet.mobility.base.MobilityBase;
import inet.mobility.base.MovingMobilityBase;
import Global.Mobility.Base_IMobility;

//module Sat_Mob_SatelliteMobility extends MobilityBase
simple Sat_Mob_SatelliteMobility extends MovingMobilityBase like Base_IMobility
{
    parameters:        
        //@class("mobility::sat_mob");
        @class(mobility::Sat_Mob_SatelliteMobility);
        @display("i=block/cogwheel");
        //bool faceForward = default(true);
		bool displaySpanArea = default(false);
        int effectiveSlant = default(0);
}

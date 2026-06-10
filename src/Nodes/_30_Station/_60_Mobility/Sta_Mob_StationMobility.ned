package Nodes._30_Station._60_Mobility;

import inet.mobility.static.StationaryMobility;

simple Sta_Mob_StationMobility extends StationaryMobility
{
    parameters:
        double latitude;
        double longitude;
        initFromDisplayString = false;
        updateFromDisplayString = false;
        @class(mobility::Sta_Mob_StationMobility);
}

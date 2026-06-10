package Nodes._20_Satellite._60_Mobility;

module SatelliteMobility
{
    parameters:
        double inclination @unit(deg) = default(0deg);
        double ascNode @unit(deg) = default(0deg);
        double meanMotion @unit(deg/s) = default(0deg/1s);
        @display("i=block/cogwheel");
}

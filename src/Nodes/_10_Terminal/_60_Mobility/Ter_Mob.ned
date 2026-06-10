package Nodes._10_Terminal._60_Mobility;

import inet.mobility.static.StationaryMobility;

//
// Initializes the initial positions of LoRa nodes.
// Two modes are available:
// 1. Uniform distribution: nodes are randomly placed around a center point 
//    within a given radius (set deploymentRadius > 0)
// 2. Exact position: node is placed at exact centerLatitude/centerLongitude
//    coordinates (set deploymentRadius = 0)
//
simple Ter_Mob extends StationaryMobility
{
    parameters:
        @class(mobility::Ter_Mob);
        double centerLatitude = default(-15.0);
        double centerLongitude = default(-56.0);
        double deploymentRadius = default(200.0);  // Set to 0 for exact positioning
        initFromDisplayString = false;
        updateFromDisplayString = false;
        @display("i=block/cogwheel");
}

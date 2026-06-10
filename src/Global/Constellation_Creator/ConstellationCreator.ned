package Global.Constellation_Creator;

simple ConstellationCreator{
    parameters:
        @class(Global::constellation::ConstellationCreator);
        bool enabled = default(false); // Set to false when using TLE files
        int planeCount = default(0); // Required when enabled=true
        int interPlaneSpacing = default(0); // Required when enabled=true, aswell required for TopologyControl
        int raanSpread = default(360); // 180 for Star, 360 for Delta
        double inclination = default(86.4);
        int altitude = default(780);
        int baseYear = default(20);
        double baseDay = default(0.8292315);
        int epochYear = default(20);
        double epochDay = default(0.8194444);
        double eccentricity = default(0.000001);
}


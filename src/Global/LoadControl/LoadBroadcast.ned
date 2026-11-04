package Global.LoadControl;

// Satellite-side broadcast admission control. See LoadBroadcast.h.
simple LoadBroadcast
{
    parameters:
        @class(loadcontrol::LoadBroadcast);

        bool enabled = default(false);       // inert unless true
        double window @unit(s) = default(10s);

        double targetBursts = default(0);    // setpoint in detected bursts per window; 0 keeps u at fixedU
        double fixedU = default(1);          // open-loop admission factor, for thinning sweeps
        double uMin = default(0.02);
        double ewmaAlpha = default(0.5);     // weight of the newest window in the load estimate
        double gain = default(0.5);          // correction exponent; 1 corrects the full error in one step
        double maxRise = default(1.5);       // largest upward factor per window
        int bits = default(0);               // broadcast quantisation; 0 keeps full precision
        int delayWindows = default(1);       // windows between a measurement and devices holding the value
}

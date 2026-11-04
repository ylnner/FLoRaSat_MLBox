// Standalone checks for src/Global/LoadControl/LoadControlCore.h, kept outside src/ because the
// build scans that tree with --deep and a second main() would break it.
//   clang++ -std=c++17 -Isrc/Global/LoadControl tests/loadcontrol_core_test.cpp -o /tmp/lc_test && /tmp/lc_test

#include <cassert>
#include <cstdio>

#include "LoadControlCore.h"

using namespace loadcontrol;

static bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Plant: uncontrolled load B plus u * C, with the loop's own delay and smoothing.
static double settle(ControllerParams p, double B, double C, int windows = 400)
{
    AdmissionController c(p);
    for (int i = 0; i < windows; i++)
        c.step(B + c.published() * C);
    return c.published();
}

int main()
{
    ControllerParams p;
    p.target = 100;

    // Fully controllable: u settles at target / demand.
    assert(near(settle(p, 0, 400), 0.25, 1e-3));

    // Partly controllable: the fixed point is (target - B) / C, with no estimate of B needed.
    assert(near(settle(p, 60, 400), 0.10, 1e-3));

    // Uncontrolled load above the target: no authority left, u rests at the floor.
    assert(near(settle(p, 150, 400), p.uMin, 1e-9));

    // Demand below the target: control releases completely.
    assert(near(settle(p, 0, 50), 1.0, 1e-9));

    // A longer dead time still settles at this gain.
    ControllerParams slow = p;
    slow.delayWindows = 4;
    slow.gain = 0.25;
    assert(near(settle(slow, 0, 400), 0.25, 1e-2));

    // An idle channel at the start cannot jump u: the rise per window is capped.
    {
        ControllerParams q = p;
        q.delayWindows = 0;
        AdmissionController c(q);
        for (int i = 0; i < 40; i++)
            c.step(1000);
        double low = c.published();
        assert(low < 0.2);
        c.step(0);
        assert(c.published() <= low * q.maxRise + 1e-12);
    }

    // Open loop holds fixedU whatever is measured.
    {
        ControllerParams q;
        q.fixedU = 0.4;
        AdmissionController c(q);
        for (int i = 0; i < 5; i++)
            c.step(1000);
        assert(near(c.published(), 0.4, 1e-12));
    }

    // The published value lags by delayWindows.
    {
        ControllerParams q = p;
        q.delayWindows = 2;
        AdmissionController c(q);
        c.step(1000);
        assert(near(c.published(), 1.0, 1e-12));
        c.step(1000);
        assert(near(c.published(), 1.0, 1e-12));
        c.step(1000);
        assert(c.published() < 1.0);
    }

    // Quantisation: 2^bits levels between uMin and 1, both ends reachable.
    {
        ControllerParams q = p;
        q.bits = 2;
        q.delayWindows = 0;
        AdmissionController c(q);
        assert(near(c.published(), 1.0, 1e-12));
        for (int i = 0; i < 100; i++)
            c.step(1e6);
        assert(near(c.published(), q.uMin, 1e-12));
        double width = (1.0 - q.uMin) / 3.0;
        for (int i = 0; i < 400; i++) {
            c.step(400 * c.published());
            double k = (c.published() - q.uMin) / width;
            assert(near(k, std::round(k), 1e-9));
        }
    }

    // Threshold table: interpolation, ends held, higher admission means a lower threshold.
    {
        ThresholdTable t({{1.0, 0.0}, {0.5, 0.4}, {0.1, 0.9}});
        assert(near(t.tau(0.5), 0.4, 1e-12));
        assert(near(t.tau(0.75), 0.2, 1e-12));
        assert(near(t.tau(0.05), 0.9, 1e-12));
        assert(near(t.tau(1.0), 0.0, 1e-12));
        assert(t.tau(0.2) > t.tau(0.8));
    }

    // Gate probability.
    assert(gateProbability(0.6, 0.5, 0) == 1.0);
    assert(gateProbability(0.4, 0.5, 0) == 0.0);
    assert(near(gateProbability(0.5, 0.5, 0.1), 0.5, 1e-12));
    assert(gateProbability(0.9, 0.5, 0.1) > 0.95);

    // Logit scale: symmetric, and finite at the ends.
    assert(near(logit(0.5), 0.0, 1e-12));
    assert(near(logit(0.9), -logit(0.1), 1e-9));
    assert(std::isfinite(logit(0.0)) && std::isfinite(logit(1.0)));

    // Trajectory skip: always transmit with no devices, less as N grows, more with a larger pSkip.
    assert(near(skipTransmitProbability(0, 100), 1.0, 1e-12));
    assert(skipTransmitProbability(200, 100) < skipTransmitProbability(100, 100));
    assert(skipTransmitProbability(100, 400) > skipTransmitProbability(100, 100));
    assert(skipTransmitProbability(1e6, 100) < 1e-6);

    // Beaconed count: first window taken as is, then smoothed, and held back by the delay.
    {
        DelayedAverage d(0.5, 1);
        d.step(100);
        assert(near(d.published(), 0.0, 1e-12));
        d.step(200);
        assert(near(d.published(), 100.0, 1e-12));
        d.step(200);
        assert(near(d.published(), 150.0, 1e-12));
    }

    std::puts("loadcontrol core: all checks passed");
    return 0;
}

#ifndef GLOBAL_LOADCONTROL_LOADCONTROLCORE_H_
#define GLOBAL_LOADCONTROL_LOADCONTROLCORE_H_

// Broadcast admission control and score gating for ACK-less random access.
// Plain C++ with no simulator dependency, so tests/loadcontrol_core_test.cpp can build it alone.
//
// The satellite counts detected bursts per window and publishes an admission factor u in
// [uMin, 1]. A device that hears u spends it either on a coin (access barring) or on its best
// opportunities, using a table that maps u to a score threshold.

#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace loadcontrol {

struct ControllerParams {
    double target = 0;      // setpoint in detected bursts per window; <= 0 keeps u at fixedU
    double fixedU = 1;      // open-loop admission factor
    double uMin = 0.02;     // floor, so the channel is never closed completely
    double ewmaAlpha = 0.5; // weight of the newest window in the load estimate
    double gain = 0.5;      // exponent of the correction; 1 corrects the full error in one step
    double maxRise = 1.5;   // largest upward factor per window
    int bits = 0;           // broadcast quantisation; 0 keeps full precision
    int delayWindows = 1;   // windows between a measurement and the value reaching devices
};

// Multiplicative integrator: u <- clip(u * (target / estimate)^gain).
// It needs no split between the controlled and uncontrolled load. With an uncontrolled load B and
// a controllable demand C, the fixed point of B + u*C = target is u = (target - B) / C, and u
// settles at uMin when B alone exceeds the target.
class AdmissionController {
  public:
    explicit AdmissionController(const ControllerParams &params)
        : p(params), u(params.target > 0 ? 1.0 : params.fixedU), out(quantise(u))
    {
    }

    // Closes one window and returns the value devices hold for the next one.
    double step(double observedBursts)
    {
        if (p.target > 0) {
            estimate = primed ? p.ewmaAlpha * observedBursts + (1 - p.ewmaAlpha) * estimate
                              : observedBursts;
            primed = true;
            double factor = std::pow(p.target / std::max(estimate, 1e-9), p.gain);
            u = std::clamp(u * std::min(factor, p.maxRise), p.uMin, 1.0);
        }
        pipeline.push_back(quantise(u));
        if ((int)pipeline.size() > p.delayWindows) {
            out = pipeline.front();
            pipeline.pop_front();
        }
        return out;
    }

    double published() const { return out; }
    double internal() const { return u; }
    double loadEstimate() const { return estimate; }

  private:
    double quantise(double value) const
    {
        if (p.bits <= 0)
            return value;
        double levels = std::pow(2.0, p.bits) - 1;
        double width = (1.0 - p.uMin) / levels;
        return std::clamp(p.uMin + std::round((value - p.uMin) / width) * width, p.uMin, 1.0);
    }

    ControllerParams p;
    double u;
    double out;
    double estimate = 0;
    bool primed = false;
    std::deque<double> pipeline;
};

// Maps an admission factor to a score threshold, by linear interpolation between stored points.
class ThresholdTable {
  public:
    ThresholdTable() = default;
    explicit ThresholdTable(std::vector<std::pair<double, double>> points) : rows(std::move(points))
    {
        std::sort(rows.begin(), rows.end());
    }

    // CSV with a header line, then "u,tau" rows.
    static ThresholdTable fromCsv(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("cannot open threshold table: " + path);
        std::vector<std::pair<double, double>> points;
        std::string line;
        std::getline(file, line);
        while (std::getline(file, line)) {
            std::stringstream fields(line);
            std::string a, b;
            if (std::getline(fields, a, ',') && std::getline(fields, b, ','))
                points.emplace_back(std::stod(a), std::stod(b));
        }
        if (points.empty())
            throw std::runtime_error("empty threshold table: " + path);
        return ThresholdTable(std::move(points));
    }

    bool empty() const { return rows.empty(); }

    double tau(double u) const
    {
        if (u <= rows.front().first)
            return rows.front().second;
        if (u >= rows.back().first)
            return rows.back().second;
        auto hi = std::lower_bound(rows.begin(), rows.end(), std::make_pair(u, -INFINITY));
        auto lo = hi - 1;
        double w = (u - lo->first) / (hi->first - lo->first);
        return lo->second + w * (hi->second - lo->second);
    }

  private:
    std::vector<std::pair<double, double>> rows; // sorted by u
};

// Transmission probability around a threshold. softness <= 0 gives a hard gate; a soft gate adds
// dither, so devices with similar geometry do not all switch at the same instant.
inline double gateProbability(double score, double tau, double softness)
{
    if (softness <= 0)
        return score >= tau ? 1.0 : 0.0;
    return 1.0 / (1.0 + std::exp(-(score - tau) / softness));
}

} // namespace loadcontrol

#endif

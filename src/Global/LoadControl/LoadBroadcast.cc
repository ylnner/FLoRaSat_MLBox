#include "LoadBroadcast.h"

using namespace omnetpp;

namespace loadcontrol {

Define_Module(LoadBroadcast);

LoadBroadcast::~LoadBroadcast()
{
    cancelAndDelete(windowTimer);
}

void LoadBroadcast::initialize()
{
    ControllerParams p;
    p.target = par("targetBursts").doubleValue();
    p.fixedU = par("fixedU").doubleValue();
    p.uMin = par("uMin").doubleValue();
    p.ewmaAlpha = par("ewmaAlpha").doubleValue();
    p.gain = par("gain").doubleValue();
    p.maxRise = par("maxRise").doubleValue();
    p.bits = par("bits").intValue();
    p.delayWindows = par("delayWindows").intValue();
    controller = std::make_unique<AdmissionController>(p);

    window = par("window");
    observedVector.setName("lcObservedBursts");
    estimateVector.setName("lcLoadEstimate");
    admissionVector.setName("lcAdmissionFactor");

    if (par("enabled").boolValue()) {
        windowTimer = new cMessage("lcWindow");
        scheduleAt(simTime() + window, windowTimer);
    }
}

void LoadBroadcast::reportBurst()
{
    windowBursts++;
    totalBursts++;
}

void LoadBroadcast::handleMessage(cMessage *msg)
{
    double u = controller->step((double)windowBursts);

    observedVector.record((double)windowBursts);
    estimateVector.record(controller->loadEstimate());
    admissionVector.record(u);

    windows++;
    if (u < 1.0)
        windowsBelowOne++;
    windowBursts = 0;

    scheduleAt(simTime() + window, msg);
}

void LoadBroadcast::finish()
{
    recordScalar("lcTotalBursts", totalBursts);
    recordScalar("lcWindows", windows);
    recordScalar("lcWindowsBelowOne", windowsBelowOne);
}

} // namespace loadcontrol

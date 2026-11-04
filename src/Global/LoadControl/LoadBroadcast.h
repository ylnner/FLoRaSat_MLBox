#ifndef GLOBAL_LOADCONTROL_LOADBROADCAST_H_
#define GLOBAL_LOADCONTROL_LOADBROADCAST_H_

#include <memory>
#include <omnetpp.h>

#include "LoadControlCore.h"

namespace loadcontrol {

// Satellite side of the broadcast loop, one instance per network. Devices report the bursts the
// satellite would detect and read the published admission factor directly, so the air interface
// is bypassed. Its cost is charged afterwards, as bits per update and listening time per device.
class LoadBroadcast : public omnetpp::cSimpleModule {
  public:
    ~LoadBroadcast() override;

    double admissionFactor() const { return controller->published(); }
    void reportBurst();

  protected:
    void initialize() override;
    void handleMessage(omnetpp::cMessage *msg) override;
    void finish() override;

  private:
    std::unique_ptr<AdmissionController> controller;
    omnetpp::cMessage *windowTimer = nullptr;
    omnetpp::simtime_t window;

    long windowBursts = 0;
    long totalBursts = 0;
    long windows = 0;
    long windowsBelowOne = 0;

    omnetpp::cOutVector observedVector;
    omnetpp::cOutVector estimateVector;
    omnetpp::cOutVector admissionVector;
};

} // namespace loadcontrol

#endif

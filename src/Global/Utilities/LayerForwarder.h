#pragma once

#include <omnetpp.h>

namespace florasat {

class LayerForwarder : public omnetpp::cSimpleModule {
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
};

} // namespace florasat

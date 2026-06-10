#ifndef NODES__30_STATION__10_PHYSICAL_STA_PHY_H_
#define NODES__30_STATION__10_PHYSICAL_STA_PHY_H_

#include <omnetpp.h>
#include "Global/Utilities/LayerForwarder.h"

using namespace omnetpp;

namespace station {

class Sta_Phy : public cSimpleModule{
protected:
    virtual void handleMessage(cMessage *msg) override;
};

} // namespace station

#endif /* NODES__30_STATION__10_PHYSICAL_STA_PHY_H_ */

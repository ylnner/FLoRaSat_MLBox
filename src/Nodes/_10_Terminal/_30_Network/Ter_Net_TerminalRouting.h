/*
 * Ter_Net_TerminalRouting.h
 *
 *  Created on: Feb 4, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__30_NETWORK_TER_NET_TERMINALROUTING_H_
#define NODES__10_TERMINAL__30_NETWORK_TER_NET_TERMINALROUTING_H_

#include <omnetpp.h>

#include "Global/Utilities/Constants.h"
#include "Global/Utilities/ISLDirection.h"
#include "Global/Utilities/ISLState.h"
#include "Global/Utilities/PositionAwareBase.h"
#include "Global/Utilities/Utils.h"
#include "Global/Base/_30_Network/Ter_Base_Routing.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "Global/Utilities/Utils.h"

using namespace omnetpp;
using namespace inet;
using namespace routing;

namespace routing {
    class Ter_Base_Routing;
}

namespace terminal{

class Ter_Net_TerminalRouting : public cSimpleModule {

protected:
    // Reference to routing module
    routing::Ter_Base_Routing *routingModule = nullptr;


    //virtual void finish() override;
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
};

}



#endif /* NODES__10_TERMINAL__30_NETWORK_TER_NET_TERMINALROUTING_H_ */

/*
 * Ter_Null_Routing.h
 *
 *  Created on: Feb 13, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_BASE__30_NETWORK_TER_NULL_ROUTING_H_
#define GLOBAL_BASE__30_NETWORK_TER_NULL_ROUTING_H_
#include <omnetpp.h>

#include "Global/Base/_30_Network/Ter_Base_Routing.h"
using namespace omnetpp;

namespace routing{

class Ter_Null_Routing : public Ter_Base_Routing{
protected:
    int numInitStages() const override {return inet::NUM_INIT_STAGES;}
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};


}




#endif /* GLOBAL_BASE__30_NETWORK_TER_NULL_ROUTING_H_ */

/*
 * Null_Routing.h
 *
 *  Created on: Feb 13, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_BASE__30_NETWORK_NULL_ROUTING_H_
#define GLOBAL_BASE__30_NETWORK_NULL_ROUTING_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace routing{

class Null_Routing : public cSimpleModule{
protected:
    virtual void handleMessage(cMessage *msg) override;
};


}
#endif /* GLOBAL_BASE__30_NETWORK_NULL_ROUTING_H_ */

/*
 * Null_Routing.cc
 *
 *  Created on: Feb 13, 2026
 *      Author: ylnner
 */




#include "Null_Routing.h"

namespace routing{
Define_Module(Null_Routing);


void Null_Routing::handleMessage(cMessage * msg){
    delete msg;
}
}

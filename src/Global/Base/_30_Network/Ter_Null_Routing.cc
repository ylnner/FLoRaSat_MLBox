/*
 * Ter_Null_Routing.cc
 *
 *  Created on: Feb 13, 2026
 *      Author: ylnner
 */




#include "Ter_Null_Routing.h"

namespace routing{
Define_Module(Ter_Null_Routing);

void Ter_Null_Routing::initialize(int stage){
    // Initialize function overwritten to avoid load topologyControl module
}

void Ter_Null_Routing::handleMessage(cMessage * msg){
    delete msg;
}
}

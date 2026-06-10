#include "LayerForwarder.h"

#include <cstring>

using namespace omnetpp;

namespace florasat {
Define_Module(LayerForwarder);

void LayerForwarder::initialize() {
    // Simple passthrough module, no special initialization needed
}

void LayerForwarder::handleMessage(cMessage *msg) {
    cGate *arrival = msg->getArrivalGate();
    if (!arrival) {
        EV_WARN << "Message arrived without arrival gate, dropping\n";
        delete msg;
        return;
    }

    const char *arrivalName = arrival->getName();

    // Simple passthrough: upperIn -> lowerOut, lowerIn -> upperOut
    if (strcmp(arrivalName, "upperIn") == 0) {
        if (hasGate("lowerOut")) {
            send(msg, "lowerOut");
        } else {
            EV_WARN << "No lowerOut gate, dropping message\n";
            delete msg;
        }
    } else if (strcmp(arrivalName, "lowerIn") == 0) {
        if (hasGate("upperOut")) {
            send(msg, "upperOut");
        } else {
            EV_WARN << "No upperOut gate, dropping message\n";
            delete msg;
        }
    } else {
        EV_WARN << "Message received on unexpected gate '" << arrivalName << "', dropping\n";
        delete msg;
    }
}

} // namespace florasat

#include "Sta_Phy.h"

namespace station {

Define_Module(Sta_Phy);

void Sta_Phy::handleMessage(cMessage *msg) {
    cGate *arrival = msg->getArrivalGate();
    if (!arrival) {
        EV_WARN << "Message arrived without arrival gate, dropping\n";
        delete msg;
        return;
    }

    const char *arrivalName = arrival->getName();
    
    // Handle satellite link messages
    if (strncmp(arrivalName, "satLink$i", 9) == 0) {
        // Message from satellite link, send up to MAC layer
        if (hasGate("upperOut")) {
            send(msg, "upperOut");
        } else {
            EV_WARN << "No upperOut gate, dropping message from satellite link\n";
            delete msg;
        }
    }
    // Handle messages from upper layer (MAC) going down
    else if (strcmp(arrivalName, "upperIn") == 0) {
        // For now, just drop it - you may need to implement routing logic
        // to determine which satellite link to use
        EV_WARN << "Message from upper layer - routing not yet implemented\n";
        delete msg;
    }
}

} // namespace station

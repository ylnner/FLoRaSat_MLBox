
#ifndef __FLORA_SATELLITE_ISLINTERFACE_H_
#define __FLORA_SATELLITE_ISLINTERFACE_H_

#include <omnetpp.h>
#include <string>

/////////#include "PacketHandlerRouting.h"
//#include "SatelliteRouting.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/queueing/base/ActivePacketSinkBase.h"
#include "inet/common/TimeTag_m.h"
#include "Global/Mobility/INorad.h"
// #include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_NoradA.h"
////////////#include "routing/RoutingBase.h"
////////////#include "topologycontrol/TopologyControlBase.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"
#include "Global/Messages/_30_Network/Net_QueueInsertionTimeTag_m.h"

using namespace omnetpp;
using namespace inet::queueing;
using namespace inet;;

namespace satellite {

class ISLInterface : public ClockUserModuleMixin<queueing::ActivePacketSinkBase> {
   protected:
    ///////////PacketHandlerRouting *sender = nullptr;               // cached pointer
    cGate *islOutGate = nullptr;                  // cached pointer
    ClockEvent *collectionTimer = nullptr;
    int numDroppedPackets = 0;

   protected:
    // cModule
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    // packet sink
    virtual void collectPacket();

   public:
    virtual ~ISLInterface() { cancelAndDeleteClockEvent(collectionTimer); }

    virtual void handleCanPullPacketChanged(cGate *gate) override;
    virtual void handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) override;

    std::string resolveDirective(char directive) const override;
};
}  // namespace satellite


#endif  // __FLORA_SATELLITE_ISLINTERFACE_H_


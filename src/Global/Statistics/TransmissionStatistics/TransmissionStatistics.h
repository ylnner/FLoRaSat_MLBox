/*
 * TransmissionStatistics.h
 *
 *  Created on: Feb 16, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_STATISTICS_TRANSMISSIONSTATISTICS_TRANSMISSIONSTATISTICS_H_
#define GLOBAL_STATISTICS_TRANSMISSIONSTATISTICS_TRANSMISSIONSTATISTICS_H_

#include <omnetpp.h>
#include <fstream>
#include <string>

#include "inet/common/INETUtils.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Packet.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"
#include "CstTransmissionStatisticsTag_m.h"
#include "Global/Utilities/Utils.h"
#include "Nodes/_10_Terminal/Ter.h"
#include "Nodes/_20_Satellite/Sat.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility_Standalone.h"

using namespace omnetpp;
using namespace inet;


namespace statistics{
class TransmissionStatistics : public cSimpleModule, public cListener{
private:
    simsignal_t signal_TranmissionStatisticsCorrect;
    simsignal_t signal_TranmissionStatisticsIgnoring;

    simsignal_t signal_LoRaReceiver;
public:
    TransmissionStatistics();

    void recordPacket(inet::Packet *packet, double minPower_dBm, double sensitivity_dBm, double duration);

protected:
    virtual ~TransmissionStatistics();

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

protected:
    int numPackets = 0;
    std::ofstream fileStats;
};

}


#endif /* GLOBAL_STATISTICS_TRANSMISSIONSTATISTICS_TRANSMISSIONSTATISTICS_H_ */

/*
 * Transmission_Predictor_Analytical.h
 *
 *  Created on: Jun 18, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_ANALYTICAL_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_ANALYTICAL_H_



#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include <deque>
#include <cmath>
#include "inet/common/Units.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/ScalarAnalogModelBase.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"

#include "Global/Channel/LoRaBandListening.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility_Standalone.h"
#include "Global/Channel/LoRaAnalogModel.h"
#include "Global/Medium/DSL/LoRaMedium.h"
#include "Nodes/_20_Satellite/Sat.h"

using namespace omnetpp;
using namespace inet;
using namespace channel;
using namespace mobility;

class Transmission_Predictor_Analytical : public cSimpleModule{

protected:
    double k = 20;//5;
    double alpha = 0.00001; //0.0001;
    int t_window = 2;//1;
    //double threshold = 0.00293165740758182;
    double threshold;
    //double lambda_traffic = 1;
    //{'T_window': 1, 'alpha': 0.0001, 'k': 5}

protected:
    double computeReceptionPower();
    double getSensitivity(int loraSF);

public:
    //double getLambdaTraffic();
    //void setLambdaTraffic(double n_lambda_traffic);
    //void initialize(int stage) override;
    double predict(int loraSF, double dopplerShift, double timeToNextPacket);

};



#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_ANALYTICAL_H_ */

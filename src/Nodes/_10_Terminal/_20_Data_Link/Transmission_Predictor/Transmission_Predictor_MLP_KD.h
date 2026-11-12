/*
 * Transmission_Predictor_MLP_KD.h
 *
 *  Created on: Nov 11, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_KD_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_KD_H_


#include <torch/script.h>  // LibTorch
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include <deque>
#include <vector>
#include "Global/Base/MLBox/Base_MLBox.h"

using namespace omnetpp;
using namespace inet;
using namespace mlbox;

namespace mlbox{
    class Transmission_Predictor_MLP_KD: public Base_MLBox{
    private:
        int num_features = 8;

        // MLP
        std::vector<double> means;
        std::vector<double> scales;


    public:
        void initialize(int stage) override;
        void loadModel() override;
        void loadScaler();
        std::vector<double> predict(const std::vector<double>& features) override;
        std::vector<double> scaleFeatures(const std::vector<double>& rawFeatures);
    };
}




#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_KD_H_ */

/*
 * Transmission_Predictor_MLP.h
 *
 *  Created on: Aug 3, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_H_


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
    class Transmission_Predictor_MLP: public Base_MLBox{
    private:
        //torch::jit::script::Module model;
        int num_features = 8;
        std::string scalerPath;
        //double threshold = 0.6074715;

        // MLP
        const std::vector<double> means = {
                48.59192333333333, 11.996563166666666, 9.85826152279762,
                0.0204108, 11.116666666666667, -582.7580839379167,
                715.0, 214.82142857142858
        };

        const std::vector<double> scales = {
                8.249025798831575, 13.106201991101578, 17.685909582817963,
                0.0039332991190602325, 0.7765665171481163, 14675.439418645408,
                88.94219631712659, 75.23750659052428
        };



    protected:
        torch::jit::script::Module scaler;

    public:
        void initialize(int stage) override;
        void loadModel() override;
        std::vector<double> predict(const std::vector<double>& features) override;
        std::vector<double> scaleFeatures(const std::vector<double>& rawFeatures);
        double scaleDeltaTime (double raw_delta_time);
    };
}


#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_H_ */

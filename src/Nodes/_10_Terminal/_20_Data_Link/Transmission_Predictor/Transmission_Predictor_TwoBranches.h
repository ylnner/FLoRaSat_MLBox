/*
 * Transmission_Predictor_TwoBranches.h
 *
 *  Created on: Aug 5, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_TWOBRANCHES_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_TWOBRANCHES_H_



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
    class Transmission_Predictor_TwoBranches: public Base_MLBox{
    private:
        //torch::jit::script::Module model;
        int seq_length = 8;
        int num_features = 10;
        std::deque<std::vector<double>> window_buffer;
        std::string scalerPath;
        //double threshold = 0.6437982;
        //double threshold;

        // TwoBranches
        const std::vector<double> means = {
                48.46870833333334, 12.207882466666668, 9.779563074523809,
                0.020494499999999995, 11.083333333333334, -532.5326136885119,
                715.0, 214.82142857142858
        };


        const std::vector<double> scales = {
                8.471769007558608, 13.381336166343383, 17.638442874411172,
                0.0035565790796775487, 0.7808471183414987, 14628.18010788066,
                88.94219631712659, 75.23750659052428
        };

        const double mean_delta_t = 61.15129680631869;
        const double scale_delta_t = 40.82371361880287;


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



#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_TWOBRANCHES_H_ */

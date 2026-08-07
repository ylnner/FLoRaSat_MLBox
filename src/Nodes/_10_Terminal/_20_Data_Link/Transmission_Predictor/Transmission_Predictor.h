/*
 * Transmission_Predictor.h
 *
 *  Created on: May 20, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_

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
    class Transmission_Predictor: public Base_MLBox{
    private:
        //torch::jit::script::Module model;
        int seq_length = 8;
        int num_features = 9;
        std::deque<std::vector<double>> window_buffer;
        std::string scalerPath;
        //double threshold = 0.6437982;
        //double threshold;

        // Transformer
        const std::vector<double> means = {
                48.55624, 11.99590675, 9.738305531309523,
                0.0206337, 11.033333333333333, -637.2126753574801,
                715.0, 214.82142857142858
        };


        const std::vector<double> scales = {
                8.630896169328729, 13.408186415520879, 17.75413475236267,
                0.003885927908492385, 0.8359957469322968, 14639.724227730489,
                88.94219631712659, 75.23750659052429
        };

        const double mean_delta_t = 61.31933958104395;
        const double scale_delta_t = 40.97636157062998;

        bool coldStart;
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


#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_ */

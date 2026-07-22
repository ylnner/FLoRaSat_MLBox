/*
 * Transmission_Predictor_BiLSTM.h
 *
 *  Created on: Jun 11, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_BILSTM_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_BILSTM_H_


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
    class Transmission_Predictor_BiLSTM: public Base_MLBox{
    private:
        //torch::jit::script::Module model;
        int seq_length = 8;
        int num_features = 9;
        std::deque<std::vector<double>> window_buffer;
        std::string scalerPath;
        //double threshold = 0.6074715;

        // BiLSTM
        const std::vector<double> means = {
                48.66285666666667, 12.06440095, 9.757231017083333,
                0.021542999999999996, 10.883333333333333, -510.220131820635,
                715.0, 214.82142857142858
        };

        const std::vector<double> scales = {
                8.63133040331108, 13.23671150419024, 17.712074870832836,
                0.003646183758397264, 0.7547994582816169, 14677.14552592761,
                88.94219631712659, 75.23750659052429
        };

        const double mean_delta_t = 61.42395316941391;
        const double scale_delta_t = 41.06019422261135;

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


#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_BILSTM_H_ */

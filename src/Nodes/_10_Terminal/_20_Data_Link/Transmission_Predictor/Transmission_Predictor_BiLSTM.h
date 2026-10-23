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
                48.65901333333334, 12.03978895, 9.818097488670633,
                0.020066399999999998, 10.933333333333334, -447.96115595273824,
                715.0, 214.82142857142858
        };

        const std::vector<double> scales = {
                8.67688779738962, 13.329355167546092, 17.71640489314641,
                0.0037267134099632617, 0.813770374382247, 14642.787636931365,
                88.94219631712659, 75.23750659052429
        };

        const double mean_delta_t = 61.01802962820513;
        const double scale_delta_t = 40.76300672650946;

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

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
        int seq_length = 16;
        int num_features = 9;
        std::deque<std::vector<double>> window_buffer;
        std::string scalerPath;

        // Transformer
        /*
        const std::vector<double> means = {
                48.474721364653256, 12.27788855033557, 8.380613159955256,
                0.020485389261744966, 11.08165548098434, 1482.4965659843397,
                712.2818791946308, 212.49161073825502
        };


        const std::vector<double> scales = {
                8.470537422002296, 13.373158421026409, 18.80787765213579,
                0.003554926938735937, 0.7808095021335768, 14601.095273656092,
                88.56515294364773, 73.66997373619341
        };
        */
        // BiLSTM

        const std::vector<double> means = {
                48.474721364653256, 12.27788855033557, 8.380613159955256,
                0.020485389261744966, 11.08165548098434, 1482.4965659843397,
                712.2818791946308, 212.49161073825502
        };


        const std::vector<double> scales = {
                8.470537422002296, 13.373158421026409, 18.80787765213579,
                0.003554926938735937, 0.7808095021335768, 14601.095273656092,
                88.56515294364773, 73.66997373619341
        };

    protected:
        torch::jit::script::Module scaler;

    public:
        void initialize(int stage) override;
        void loadModel() override;
        std::vector<double> predict(const std::vector<double>& features) override;
        std::vector<double> scaleFeatures(const std::vector<double>& rawFeatures);
    };
}


#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_ */

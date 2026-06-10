/*
 * Transmission_Predictor.h
 *
 *  Created on: May 5, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_
#define GLOBAL_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_

/*
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
        int num_features = 8;
        std::deque<std::vector<float>> window_buffer;
        std::string scalerPath;

    protected:
        torch::jit::script::Module scaler;

    public:
        void initialize(int stage) override;
        void loadModel() override;
        std::vector<float> predict(std::vector<float>* features) override;
    };
}

*/
#endif /* GLOBAL_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_H_ */

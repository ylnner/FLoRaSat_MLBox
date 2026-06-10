/*
 * Base_MLBox.h
 *
 *  Created on: Apr 9, 2026
 *      Author: ylnner
 */

#ifndef GLOBAL_BASE_MLBOX_BASE_MLBOX_H_
#define GLOBAL_BASE_MLBOX_BASE_MLBOX_H_

#include <torch/script.h>
#include <omnetpp.h>
#include <vector>
#include <string>
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarReception.h"

namespace mlbox{
using namespace omnetpp;
using namespace inet;

class Base_MLBox : public cSimpleModule {

protected:
    std::string modelPath;
    torch::jit::script::Module model;
    bool isModelLoaded = false;

public:
    /**
     * @brief Main inference method.
     * Receives a feature vector prepared by the requesting node.
     * Returns a floating-point vector (useful if the model produces multiple outputs, e.g., class probabilities).
     */
    virtual std::vector<double> predict(const std::vector<double>& features);

    /**
     * @brief Loads the model into memory. Useful if reloading is required at runtime.
     */
    virtual void loadModel();

    /**
     * @brief Indicates whether the model is initialized and ready for inference.
     */
    bool isReady() const { return isModelLoaded; }

protected:
    /*
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    */
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

};

}


#endif /* GLOBAL_BASE_MLBOX_BASE_MLBOX_H_ */

/*
 * Transmission_Predictor_MLP_SFTP.h
 *
 *  Created on: Nov 16, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_SFTP_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_SFTP_H_



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
    class Transmission_Predictor_MLP_SFTP: public Base_MLBox{
    private:
        int NUM_INPUTS = 6;
        int NUM_COMBOS = 9;
        // MLP
        std::vector<double> means;
        std::vector<double> scales;
        double minProbability = 0; // Minma probabilidad, bajo esto no transmite


    public:
        void initialize(int stage) override;
        void loadModel() override;
        void loadScaler();
        std::vector<double> predict(const std::vector<double>& features) override;
        std::vector<double> scaleFeatures(const std::vector<double>& rawFeatures);

        // Chosen configuration. -1 if it lower that minProbability
        int chooseCombo(const std::vector<double>& probs) const;
        static int comboSF(int combo) { return 10 + combo / 3; }
        static int comboTP(int combo) { return 12 + combo % 3; }   // dBm
    };
}



#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_SFTP_H_ */

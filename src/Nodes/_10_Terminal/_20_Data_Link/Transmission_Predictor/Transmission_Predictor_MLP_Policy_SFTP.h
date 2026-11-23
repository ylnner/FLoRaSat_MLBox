/*
 * Transmission_Predictor_MLP_Policy_SFTP.h
 *
 *  Created on: Nov 17, 2026
 *      Author: ylnner
 */

#ifndef NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_POLICY_SFTP_H_
#define NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_POLICY_SFTP_H_


#include <torch/script.h>  // LibTorch
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include <deque>
#include <vector>
#include <algorithm>
#include "Global/Base/MLBox/Base_MLBox.h"

using namespace omnetpp;
using namespace inet;
using namespace mlbox;

namespace mlbox{
    class Transmission_Predictor_MLP_Policy_SFTP: public Base_MLBox{
    private:
        static const int NUM_INPUTS  = 8;    // 6 del MLP simple + vis_idx + nrep sended
        static const int NUM_ACTIONS = 10;   // 9 combinaciones + no transmitir
        // MLP
        std::vector<double> means;
        std::vector<double> scales;
        int nPrevStates = 3;

    public:
        static const int NO_TX = 9;

        void initialize(int stage) override;
        void loadModel() override;
        void loadScaler();
        // Devuelve las 10 probabilidades (softmax), suman 1
        std::vector<double> predict(const std::vector<double>& features) override;
        std::vector<double> scaleFeatures(const std::vector<double>& rawFeatures);

        // Random action: u ~ Uniforme(0,1) dado desde fuera (RNG propio del nodo)
        int sampleAction(const std::vector<double>& probs, double u) const;
        static int comboSF(int action) { return 10 + action / 3; }
        static int comboTP(int action) { return 12 + action % 3; }   // dBm

        // n_prev que ve la red: min(n_prev, n_prev_states - 1)
       double capNPrev(int nPrev) const { return std::min(nPrev, nPrevStates - 1); }
    };
}


#endif /* NODES__10_TERMINAL__20_DATA_LINK_TRANSMISSION_PREDICTOR_TRANSMISSION_PREDICTOR_MLP_POLICY_SFTP_H_ */

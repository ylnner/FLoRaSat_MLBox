/*
 * Base_MLBox.cc
 *
 *  Created on: Apr 9, 2026
 *      Author: ylnner
 */


#include "Base_MLBox.h"

namespace mlbox{
Define_Module(Base_MLBox);

void Base_MLBox::initialize(int stage){
    if (stage == inet::INITSTAGE_LOCAL) {
        modelPath = par("modelPath").stringValue();

        if (!modelPath.empty()) {
            loadModel();
        } else {
            EV_WARN << "Base_MLBox: No route specified" << endl;
        }
    }
}

void Base_MLBox::handleMessage(cMessage *msg) {
    EV << "Base_MLBox: handleMessage, delete msg" <<endl;
    delete msg;
}


void Base_MLBox::loadModel() {
    // La clase hija (ej. para LibTorch) debe sobrescribir esto, cargar el .pt y hacer:
    // isModelLoaded = true;
    EV << "Base_MLBox::loadModel() not implemented." << endl;
}

std::vector<double> Base_MLBox::predict(const std::vector<double>& features) {
    EV << "Base_MLBox::predict() not implemented." << endl;
    return std::vector<double>();
}

}


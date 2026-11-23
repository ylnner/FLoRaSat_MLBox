/*
 * Transmission_Predictor_MLP_SFTP.cc
 *
 *  Created on: Nov 16, 2026
 *      Author: ylnner
 */





#include "Transmission_Predictor_MLP_SFTP.h"

Define_Module(Transmission_Predictor_MLP_SFTP);

void Transmission_Predictor_MLP_SFTP::initialize(int stage){
    Base_MLBox::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        loadScaler();
        minProbability = par("minProbability").doubleValue();
        if (par("modelPath").stdstringValue().empty())
            throw cRuntimeError("mlBoxMLP_SFTP: modelPath empty, el modelo no se cargo");
    }

}



void Transmission_Predictor_MLP_SFTP::loadModel(){
    // Loading model
    EV << "MLP model loaded "<<endl;
    model = torch::jit::load(modelPath);
    // Change to eval mode to predict
    model.eval();

    // Load Scaler
}

void Transmission_Predictor_MLP_SFTP::loadScaler(){
    cValueMap *sc = check_and_cast<cValueMap *>(par("scaler").objectValue());
    if (!sc->containsKey("mean") || !sc->containsKey("scale") || !sc->containsKey("inputs") || !sc->containsKey("outputs"))
        throw cRuntimeError("Error in JSON of the scaler, does not include 'outputs', 'inputs', 'mean' y 'scale'");

    std::vector<std::string> inputs = check_and_cast<cValueArray *>(sc->get("inputs").objectValue())->asStringVector();
    std::vector<std::string> outputs = check_and_cast<cValueArray *>(sc->get("outputs").objectValue())->asStringVector();
    means  = check_and_cast<cValueArray *>(sc->get("mean").objectValue())->asDoubleVector();
    scales = check_and_cast<cValueArray *>(sc->get("scale").objectValue())->asDoubleVector();


    // New order same than config.INPUTS and config.COMBO_NAMES
    const std::vector<std::string> expectedInputs  = {"dopplerShift", "elevSat", "latDev", "longDev", "alt", "raan"};
    const std::vector<std::string> expectedOutputs = {"SF10_TP12", "SF10_TP13", "SF10_TP14",
                                                      "SF11_TP12", "SF11_TP13", "SF11_TP14",
                                                      "SF12_TP12", "SF12_TP13", "SF12_TP14"};

    if (inputs != expectedInputs || means.size() != NUM_INPUTS || scales.size() != NUM_INPUTS)
        throw cRuntimeError("scaler: inputs are not in the correct order");
    if (outputs != expectedOutputs)
        throw cRuntimeError("scaler: outputs are not in the correct order");

    EV << "Scaler cargado: " << means.size() << " entradas" << endl;
}

std::vector<double> Transmission_Predictor_MLP_SFTP::scaleFeatures(const std::vector<double>& rawFeatures){
    std::vector<double> scaledFeatures(rawFeatures.size());
    for (size_t i = 0; i < rawFeatures.size(); ++i) {
        // Standard Scaler Formula: z = (x - mean) / scale
        scaledFeatures[i] = (rawFeatures[i] - means[i]) / scales[i];
    }
    return scaledFeatures;
}

std::vector<double> Transmission_Predictor_MLP_SFTP::predict(const std::vector<double>& features){
    try{
        EV << "Transmission_Predictor::predict" <<endl;
        // Casting to float
        std::vector<float> float_features;
        float_features.reserve(features.size());

        for (double value : features) {
            float_features.push_back(static_cast<float>(value));
        }


        int64_t num_features = float_features.size();
        at::Tensor input_tensor = torch::from_blob(float_features.data(),
                                        {1, num_features},
                                        torch::kFloat32).clone();

        // 1. Ver el tamaño/forma (Shape)
        EV << "Dimensiones (Sizes): " << input_tensor.sizes() << "\n";

        // 2. Ver el tipo de datos (debería ser Float)
        EV << "Tipo de datos: " << input_tensor.dtype() << "\n";

        // 3. Ver el contenido completo
        EV << "Tensor completo:\n" << input_tensor << "\n";

        torch::NoGradGuard no_grad;
        // Run model
        at::Tensor probs = torch::sigmoid(model.forward({input_tensor}).toTensor()).squeeze(0);
        EV << "Probs (sizes): " << probs.sizes() << "\n";   // debe ser [9]
        EV << "Probs:\n" << probs << "\n";


        std::vector<double> out(NUM_COMBOS);
        for (int c = 0; c < NUM_COMBOS; ++c)
            out[c] = probs[c].item<float>();

        return out;

    } catch (const std::exception& e) {
        throw cRuntimeError("MLP SF/TP: error al ejecutar el modelo: %s", e.what());   // mejor detener que seguir con 2 valores
        EV_ERROR << "Error running model: " << e.what() << "\n";
        return {1, 1.0};
    }

}

int Transmission_Predictor_MLP_SFTP::chooseCombo(const std::vector<double>& probs) const {
    // Choose best configuration
    int best = 0;
    for (int c = 1; c < NUM_COMBOS; ++c){
        if (probs[c] > probs[best])
            best = c;
    }


    if (probs[best] < minProbability)
        return -1; // returns not transmit

    return best;
}

/*
 * Transmission_Predictor_MLP_KD.cc
 *
 *  Created on: Nov 11, 2026
 *      Author: ylnner
 */




#include "Transmission_Predictor_MLP_KD.h"

Define_Module(Transmission_Predictor_MLP_KD);

void Transmission_Predictor_MLP_KD::initialize(int stage){
    Base_MLBox::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        loadScaler();
        if (par("modelPath").stdstringValue().empty())
            throw cRuntimeError("mlBoxMLPKD: modelPath empty, el modelo no se cargo");
    }

}



void Transmission_Predictor_MLP_KD::loadModel(){
    // Loading model
    EV << "MLP model loaded "<<endl;
    model = torch::jit::load(modelPath);
    // Change to eval mode to predict
    model.eval();

    // Load Scaler
}

void Transmission_Predictor_MLP_KD::loadScaler(){
    cValueMap *sc = check_and_cast<cValueMap *>(par("scaler").objectValue());
    if (!sc->containsKey("mean") || !sc->containsKey("scale") || !sc->containsKey("inputs"))
        throw cRuntimeError("Error in JSON of the scaler, does not include 'inputs', 'mean' y 'scale'");

    std::vector<std::string> inputs = check_and_cast<cValueArray *>(sc->get("inputs").objectValue())->asStringVector();
    means  = check_and_cast<cValueArray *>(sc->get("mean").objectValue())->asDoubleVector();
    scales = check_and_cast<cValueArray *>(sc->get("scale").objectValue())->asDoubleVector();

    // New order in the input
    const std::vector<std::string> expectedInputs = {"dopplerShift", "elevSat", "latDev", "longDev", "loraTP", "loraSF", "alt", "raan"};

    // El orden de las entradas del JSON debe ser el mismo que el del vector que arma Ter_Dlk_KiWan
    if (inputs != expectedInputs || means.size() != inputs.size() || scales.size() != inputs.size())
        throw cRuntimeError("scaler: inputs are not in the correct order");

    EV << "Scaler cargado: " << means.size() << " entradas, threshold = " << threshold << endl;
}

std::vector<double> Transmission_Predictor_MLP_KD::scaleFeatures(const std::vector<double>& rawFeatures){
    std::vector<double> scaledFeatures(rawFeatures.size());
    for (size_t i = 0; i < rawFeatures.size(); ++i) {
        // Standard Scaler Formula: z = (x - mean) / scale
        scaledFeatures[i] = (rawFeatures[i] - means[i]) / scales[i];
    }
    return scaledFeatures;
}

std::vector<double> Transmission_Predictor_MLP_KD::predict(const std::vector<double>& features){
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
        at::Tensor output = torch::sigmoid(model.forward({input_tensor}).toTensor());

        // Get the prediction
        float probability = output.item<float>();

        // Round to get final prediction
        bool transmit = (probability >= threshold);
        EV << "Threshold: " << threshold <<endl;
        EV << "Probability of the model: " << probability
           << " -> Decision: " << (transmit ? "TRANSMIT (1)" : "NOT TRANSMIT (0)") << "\n";

        return {(double)transmit, (double) probability};


    } catch (const std::exception& e) {
        EV_ERROR << "Error running model: " << e.what() << "\n";
        return {1, 1.0};
    }

}

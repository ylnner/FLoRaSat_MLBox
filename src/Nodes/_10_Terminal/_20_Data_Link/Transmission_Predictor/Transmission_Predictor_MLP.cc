/*
 * Transmission_Predictor_MLP.cc
 *
 *  Created on: Aug 3, 2026
 *      Author: ylnner
 */




#include "Transmission_Predictor_MLP.h"

Define_Module(Transmission_Predictor_MLP);

void Transmission_Predictor_MLP::initialize(int stage){
    Base_MLBox::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        scalerPath = par("scalerPath").stringValue();
    }
}



void Transmission_Predictor_MLP::loadModel(){
    // Loading model
    EV << "MLP model loaded "<<endl;
    model = torch::jit::load(modelPath);
    // Change to eval mode to predict
    model.eval();

    // Load Scaler
}

std::vector<double> Transmission_Predictor_MLP::scaleFeatures(const std::vector<double>& rawFeatures){
    std::vector<double> scaledFeatures(rawFeatures.size());
    for (size_t i = 0; i < rawFeatures.size(); ++i) {
        // Standard Scaler Formula: z = (x - mean) / scale
        scaledFeatures[i] = (rawFeatures[i] - means[i]) / scales[i];
        //EV << "rawFeature: " << rawFeatures[i] <<endl;
        //EV << "scaled: " << scaledFeatures[i] <<endl;
    }
    return scaledFeatures;
}

std::vector<double> Transmission_Predictor_MLP::predict(const std::vector<double>& features){
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
        at::Tensor output = model.forward({input_tensor}).toTensor();

        // Get the prediction
        float probability = output.item<float>();

        // Round to get final prediction
        bool transmit = (probability >= threshold);
        EV << "Threshold: " << threshold <<endl;
        EV << "Probability of the model: " << probability
           << " -> Decision: " << (transmit ? "TRANSMIT (1)" : "NOT TRANSMIT (0)") << "\n";


        return {(double)transmit};


    } catch (const std::exception& e) {
        EV_ERROR << "Error running model: " << e.what() << "\n";
        return {1};
    }

}


/*
 * Transmission_Predictor.cc
 *
 *  Created on: May 5, 2026
 *      Author: ylnner
 */


#include "Transmission_Predictor.h"

Define_Module(Transmission_Predictor);

void Transmission_Predictor::initialize(int stage){
    Base_MLBox::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        scalerPath = par("scalerPath").stringValue();
        coldStart = par("coldStart").boolValue();
        //threshold = par("threshold").doubleValue();
    }
}


void Transmission_Predictor::loadModel(){
    // Loading model
    EV << "Transformer model loaded "<<endl;
    model = torch::jit::load(modelPath);
    // Change to eval mode to predict
    model.eval();

    // Load Scaler
}

std::vector<double> Transmission_Predictor::scaleFeatures(const std::vector<double>& rawFeatures){
    std::vector<double> scaledFeatures(rawFeatures.size());
    for (size_t i = 0; i < rawFeatures.size(); ++i) {
        // Standard Scaler Formula: z = (x - mean) / scale
        scaledFeatures[i] = (rawFeatures[i] - means[i]) / scales[i];
        //EV << "rawFeature: " << rawFeatures[i] <<endl;
        //EV << "scaled: " << scaledFeatures[i] <<endl;
    }
    return scaledFeatures;
}

double Transmission_Predictor::scaleDeltaTime (double raw_delta_time){
    double scaled_delta = (raw_delta_time - mean_delta_t) / scale_delta_t;
    return scaled_delta;
}

std::vector<double> Transmission_Predictor::predict(const std::vector<double>& features){
    EV << "Transmission_Predictor::predict" <<endl;
    // Insert the new features to the buffer
    window_buffer.push_back(features);

    // Control the buffer size
    if (window_buffer.size() > seq_length) {
        window_buffer.pop_front();

    }

    for (size_t i = 0; i < window_buffer.size(); ++i) {
        EV << "Vector " << i << ": ";
        for (size_t j = 0; j < window_buffer[i].size(); ++j) {
            EV << window_buffer[i][j] << " ";
        }

        EV << endl;
    }

    // if is not activated control the buffer size
    if(coldStart == false){
        // If the buffer is not complete return 1
        if (window_buffer.size() < seq_length) {
            EV << "Buffer not complete (" << window_buffer.size() << "/" << seq_length
               << "). Returning 1 by default \n";
            return {1}; // Return a vector with 1
        }
    }


    // Call the model to predict
    try {
        std::vector<float> flattened;
        std::deque<std::vector<double>> buffer_copia = window_buffer;

        double lastTime = buffer_copia.back().back();
        const size_t TIME_INDEX = 8;
        EV << "lastTime: " << lastTime<<endl;

        for (auto& row : buffer_copia) {
            EV << "deltaTime: " << lastTime - row[TIME_INDEX]<<endl;
            double deltaTime = scaleDeltaTime(lastTime - row[TIME_INDEX]);
            //double deltaTime = lastTime - row[TIME_INDEX];
            EV << "deltaTimeScaled: " << deltaTime<<endl;
            row[TIME_INDEX] = deltaTime;
        }


        // Transformer
        for (const auto& vec : buffer_copia) {
            for (double value : vec) {
                flattened.push_back(static_cast<float>(value));
            }
        }


        int current_seq_length = seq_length;

        // If is activated set the shape with the current size of the buffer
        if(coldStart){
            current_seq_length = buffer_copia.size();
        }
        // Convert to a Tensor format
        at::Tensor input_tensor = torch::from_blob(flattened.data(),
                                    {1, current_seq_length, num_features},
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


        return {(double)transmit};

    } catch (const std::exception& e) {
        EV_ERROR << "Error running model: " << e.what() << "\n";
        return {1};
    }

}


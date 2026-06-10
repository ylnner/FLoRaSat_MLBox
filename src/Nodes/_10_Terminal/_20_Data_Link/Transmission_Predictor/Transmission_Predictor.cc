
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
    }
}


void Transmission_Predictor::loadModel(){
    // Loading model
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
        EV << "rawFeature: " << rawFeatures[i] <<endl;
        EV << "scaled: " << scaledFeatures[i] <<endl;
    }
    return scaledFeatures;
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

    // If the buffer is not complete return 1
    if (window_buffer.size() < seq_length) {
        EV << "Buffer not complete (" << window_buffer.size() << "/" << seq_length
           << "). Returning 1 by default \n";
        return {1}; // Return a vector with 1
    }

    // Call the model to predict
    try {
        std::vector<float> flattened;
        // Transformer
        /*
        for (const auto& vec : window_buffer) {
            for (double value : vec) {
                flattened.push_back(static_cast<float>(value));
            }
        }
        */

        //Bi Lstm
        ///*
        std::deque<std::vector<double>> buffer_copia = window_buffer;

        double lastTime = buffer_copia.back().back();
        const size_t TIME_INDEX = 8;

        for (auto& row : buffer_copia) {
            double deltaTime = lastTime - row[TIME_INDEX];
            row[TIME_INDEX] = deltaTime;
        }


        for (const auto& vec : buffer_copia) {
            for (double value : vec) {
                flattened.push_back(static_cast<float>(value));
            }
        }
        //*/


        // Convert to a Tensor format
        at::Tensor input_tensor = torch::from_blob(flattened.data(),
                                    {1, seq_length, num_features},
                                    torch::kFloat32).clone();

        //torch::NoGradGuard no_grad;
        // Run model
        at::Tensor output = model.forward({input_tensor}).toTensor();

        // Get the prediction
        float probability = output.item<float>();

        // Round to get final prediction
        bool transmit = (probability >= 0.5);

        EV << "Probability of the model: " << probability
           << " -> Decision: " << (transmit ? "TRANSMIT (1)" : "NOT TRANSMIT (0)") << "\n";


        return {(double)transmit};

    } catch (const std::exception& e) {
        EV_ERROR << "Error running model: " << e.what() << "\n";
        return {1};
    }


    /*
    std::vector<float> current_features = {
            1, 2, 3, 4, 5, 6, 7, 8,
    };

    for (int i =0; i< seq_length; i++){
        // Insert at back of the buffer
        window_buffer.push_back(current_features);
    }
    int prediction = -1;
    std::vector<float> flattened;
    for (const auto& vec : window_buffer) {
        flattened.insert(flattened.end(), vec.begin(), vec.end());
    }
    // Convert to a Tensor format
    at::Tensor input_tensor = torch::from_blob(flattened.data(),
                                {1, seq_length, num_features},
                                torch::kFloat32).clone();
    // Run model
    at::Tensor output = model.forward({input_tensor}).toTensor();
    // Get the prediction
    float probability = output.item<float>();
    // Round to get final prediction
    bool transmit = (probability >= 0.5);

    EV << "Probability of the model: " << probability
       << " -> Decision: " << (transmit ? "TRANSMIT (1)" : "NO TRANSMITIR (0)") << "\n";

    return std::vector<float>{1};
    */
}


/*
void Ter_App_MLBox::handleMessage(cMessage *msg){
    // 1. Extraer features del evento actual
    std::vector<float> current_features; //= {f1, f2, f3, f4};

    // Insert at back of the buffer
    window_buffer.push_back(current_features);

    // Maintain seq_lenght elements in the buffer
    if (window_buffer.size() > seq_length) {
        window_buffer.pop_front();
    }

    int prediction = -1;

    // Check if the buffer reach the size of seq_length to make a prediction, otherwise returns a random number
    if (window_buffer.size() == seq_length) {

        // Crear el tensor de entrada [1, seq_length, num_features]
        // Primero aplanamos el deque a un vector plano
        std::vector<float> flattened;
        for (const auto& vec : window_buffer) {
            flattened.insert(flattened.end(), vec.begin(), vec.end());
        }

        // Convertir a Tensor
        at::Tensor input_tensor = torch::from_blob(flattened.data(),
                                    {1, seq_length, num_features},
                                    torch::kFloat32).clone();

        // Ejecutar Transformer
        at::Tensor output = model.forward({input_tensor}).toTensor();


    } else {
        EV << "Buffer llenándose: " << window_buffer.size() << "/" << seq_length << "\n";
        // Decisión por defecto mientras el buffer no está listo
        prediction = intuniform(0,1);
    }

    if (prediction != -1){

    }else{

    }

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(torch::tensor({{0.5f, 1.2f, 0.8f}}));

    auto output = model.forward(inputs).toTensor();
    int decision = output.argmax(1).item<int>();

    decision ? send(msg, "out") : delete msg;
}
*/

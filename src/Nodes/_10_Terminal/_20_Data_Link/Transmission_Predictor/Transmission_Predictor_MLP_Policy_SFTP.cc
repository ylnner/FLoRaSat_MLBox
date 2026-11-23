/*
 * Transmission_Predictor_MLP_Policy_SFTP.cc
 *
 *  Created on: Nov 17, 2026
 *      Author: ylnner
 */






#include "Transmission_Predictor_MLP_Policy_SFTP.h"

Define_Module(Transmission_Predictor_MLP_Policy_SFTP);

void Transmission_Predictor_MLP_Policy_SFTP::initialize(int stage){
    Base_MLBox::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        loadScaler();
        if (par("modelPath").stdstringValue().empty())
            throw cRuntimeError("MLP_Policy_SFTP: modelPath empty, el modelo no se cargo");
    }

}



void Transmission_Predictor_MLP_Policy_SFTP::loadModel(){
    // Loading model
    EV << "MLP model loaded "<<endl;
    model = torch::jit::load(modelPath);
    // Change to eval mode to predict
    model.eval();

    // Load Scaler
}

void Transmission_Predictor_MLP_Policy_SFTP::loadScaler(){
    cValueMap *sc = check_and_cast<cValueMap *>(par("scaler").objectValue());
    if (!sc->containsKey("mean") || !sc->containsKey("scale") || !sc->containsKey("inputs") || !sc->containsKey("outputs"))
        throw cRuntimeError("Error in JSON of the scaler, does not include 'outputs', 'inputs', 'mean' y 'scale'");

    std::vector<std::string> inputs = check_and_cast<cValueArray *>(sc->get("inputs").objectValue())->asStringVector();
    std::vector<std::string> outputs = check_and_cast<cValueArray *>(sc->get("outputs").objectValue())->asStringVector();
    means  = check_and_cast<cValueArray *>(sc->get("mean").objectValue())->asDoubleVector();
    scales = check_and_cast<cValueArray *>(sc->get("scale").objectValue())->asDoubleVector();


    // New order same than config.INPUTS and config.COMBO_NAMES
    const std::vector<std::string> expectedInputs  = {"dopplerShift", "elevSat", "latDev", "longDev", "alt", "raan", "vis_idx", "n_prev"};
    const std::vector<std::string> expectedOutputs = {"SF10_TP12", "SF10_TP13", "SF10_TP14",
                                                      "SF11_TP12", "SF11_TP13", "SF11_TP14",
                                                      "SF12_TP12", "SF12_TP13", "SF12_TP14", "NO_TX"};

    if (inputs != expectedInputs || means.size() != NUM_INPUTS || scales.size() != NUM_INPUTS)
        throw cRuntimeError("scaler: inputs are not in the correct order");
    if (outputs != expectedOutputs)
        throw cRuntimeError("scaler: outputs are not in the correct order");

    // Estados de n_prev con los que se entreno la red (7_MLP_policy_nprev lo escribe en scaler.json)
    if (!sc->containsKey("n_prev_states"))
        throw cRuntimeError("scaler: falta 'n_prev_states' (exportar con 7_MLP_policy_nprev/export_best_policy.py)");

    nPrevStates = (int) sc->get("n_prev_states").doubleValue();
    if (nPrevStates < 1)
        throw cRuntimeError("scaler: n_prev_states debe ser >= 1");

    EV << "Scaler cargado: " << means.size() << " entradas" << endl;
}

std::vector<double> Transmission_Predictor_MLP_Policy_SFTP::scaleFeatures(const std::vector<double>& rawFeatures){
    std::vector<double> scaledFeatures(rawFeatures.size());
    for (size_t i = 0; i < rawFeatures.size(); ++i) {
        // Standard Scaler Formula: z = (x - mean) / scale
        scaledFeatures[i] = (rawFeatures[i] - means[i]) / scales[i];
    }
    return scaledFeatures;
}

std::vector<double> Transmission_Predictor_MLP_Policy_SFTP::predict(const std::vector<double>& features){
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
        at::Tensor probs = torch::softmax(model.forward({input_tensor}).toTensor(), 1).squeeze(0);
        EV << "Probs (sizes): " << probs.sizes() << "\n";
        EV << "Probs:\n" << probs << "\n";


        std::vector<double> out(NUM_ACTIONS);
        for (int c = 0; c < NUM_ACTIONS; ++c)
            out[c] = probs[c].item<float>();

        return out;

    } catch (const std::exception& e) {
        throw cRuntimeError("MLP policy: error al ejecutar el modelo: %s", e.what());
        return {1, 1.0};
    }

}

int Transmission_Predictor_MLP_Policy_SFTP::sampleAction(const std::vector<double>& probs, double u) const {
    // << SORTEO: se recorre la suma acumulada y se elige la accion cuyo tramo contiene u.
    // NO argmax.
    // Dependiendo de la probabilidad u (calcuada como unifom), obtiene la probabilidad q este en ese rango.
    // Si u es alto, recorre cada action y acumula sus probabilidades hasta llegar a u
    double cumulative = 0.0;
    for (int a = 0; a < NUM_ACTIONS; ++a) {
        cumulative += probs[a];
        if (u < cumulative)
            return a;
    }
    return NUM_ACTIONS - 1;   // por redondeo (suma = 0.9999999): ultima accion
}

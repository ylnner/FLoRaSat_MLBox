/*
 * Transmission_Predictor_Analytical.cc
 *
 *  Created on: Jun 18, 2026
 *      Author: ylnner
 */



#include "Transmission_Predictor_Analytical.h"

Define_Module(Transmission_Predictor_Analytical);

/*
void Transmission_Predictor_Analytical::initialize(int stage){

}
*/


/*
double Transmission_Predictor_Analytical::getLambdaTraffic(){
    return this->lambda_traffic;
}
void Transmission_Predictor_Analytical::setLambdaTraffic(double n_lambda_traffic){
    this->lambda_traffic = n_lambda_traffic;
}
*/


double Transmission_Predictor_Analytical::getSensitivity(int loraSF)
{
    //function returns sensitivity -- according to LoRa documentation, it changes with LoRa parameters
    //Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
    units::values::W sensitivity = units::values::W(math::dBmW2mW(-126.5) / 1000);
    if(loraSF == 6)
    {
        sensitivity = units::values::W(math::dBmW2mW(-121) / 1000);

    }

    if (loraSF == 7)
    {
        sensitivity = units::values::W(math::dBmW2mW(-124) / 1000);
    }

    if(loraSF == 8)
    {
        sensitivity = units::values::W(math::dBmW2mW(-127) / 1000);

    }
    if(loraSF == 9)
    {
        sensitivity = units::values::W(math::dBmW2mW(-130) / 1000);

    }
    if(loraSF == 10)
    {
        sensitivity = units::values::W(math::dBmW2mW(-133) / 1000);
    }
    if(loraSF == 11)
    {
        sensitivity = units::values::W(math::dBmW2mW(-135) / 1000);
    }
    if(loraSF == 12)
    {
        sensitivity = units::values::W(math::dBmW2mW(-137) / 1000);
    }

    double sensitivity_dBm = math::mW2dBmW(double(sensitivity.get()*1000));

    return sensitivity_dBm;
}

double Transmission_Predictor_Analytical::computeReceptionPower(){
    EV << "Transmission_Predictor_Analytical::computeReceptionPower" <<endl;

    Ter_Mob* ter_mob = check_and_cast<Ter_Mob*>(getParentModule()->getParentModule()->getParentModule()->getSubmodule("mob"));
    satellite::Sat *sat  = check_and_cast<satellite::Sat *>(getSystemModule()->getSubmodule("satellite", 0));

    Sat_Mob_SatelliteMobility_Standalone* sat_mob = check_and_cast<Sat_Mob_SatelliteMobility_Standalone*>(sat->getSubmodule("mobility"));


    double distance = sat_mob->getDistance(ter_mob->getLatitude(), ter_mob->getLongitude(), 0);
    distance = distance * 1000; // Pasar de km a metros
    EV << "distance: " << distance << endl;


    medium::LoRaMedium *radioMedium = check_and_cast<medium::LoRaMedium*>(getSystemModule()->getSubmodule("radioMedium"));
    mps propagationSpeed = radioMedium->getPropagation()->getPropagationSpeed();
    EV << "propagationSpeed: " << propagationSpeed << endl;

    auto myRadio = ter_mob->getParentModule()->getSubmodule("phy")->getSubmodule("radio");

    double transmitterAntennaGain = 1.0;

    double receiverAntennaGain = sat->getSubmodule("phyDSL")->getSubmodule("radio")->getSubmodule("antenna")->par("gain").doubleValue();
    EV << "receiverAntennaGain: " << receiverAntennaGain << endl;

    inet::Hz centerFrequency = inet::Hz(myRadio->par("centerFrequency").doubleValue());
    EV << "centerFrequency: " << centerFrequency << endl;

    int loraTP = ter_mob->getParentModule()->getSubmodule("dlk")->getSubmodule("mac")->par("LoRaTP").doubleValue();
    EV << "loraTP: " << loraTP << endl;

    //loraTag->setPower(W(math::dBmW2mW(loRaTP)));

    inet::W transmissionPower = mW(math::dBmW2mW(loraTP));
    EV << "transmissionPower: " << transmissionPower << endl;

    double pathLoss = radioMedium->getPathLoss()->computePathLoss(propagationSpeed, centerFrequency, m(distance));
    EV << "pathLoss: " << pathLoss << endl;

    double obstacleLoss = 1.0; // Línea de vista directa al espacio (LOS)
    inet::W estimatedRxPower = transmissionPower * std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
    EV << "estimatedRxPower: " << estimatedRxPower << endl;

    return math::mW2dBmW(double(estimatedRxPower.get()*1000));;
}

double Transmission_Predictor_Analytical::predict(int loraSF, double dopplerShift, double timeToNextPacket){
    /*
     * Global
     * */

    // Estimated receptionpower
    double minPowerdBm = computeReceptionPower();
    int BW = 125000;
    double sensitivitydBm = getSensitivity(loraSF);
    double base_p_link = minPowerdBm - sensitivitydBm;

    double BW_SF = BW / std::pow(2, loraSF);
    double base_p_doppler = std::abs(dopplerShift) / BW_SF;


    /*
     * Calculate P_Link
     * */
    double P_link = 1.0 / (1.0 + std::exp(-k * base_p_link));


    /*
     * Calculate P_doppler
     * */
    double P_doppler = std::exp(-alpha * base_p_doppler);


    /*
     * Calculate P_No_Collision
     * Assuming that lamda traffic = 1, the device can not have access to this data
     * */
    int packetLenBits = 80; // The package es 10B, 80bits
    int SF = loraSF;
    int CR = 4;
    int nPreamble = 8;
    double payloadBytes = static_cast<double>(packetLenBits) / 8.0;

    int payloadSymbNb = 8;
    payloadSymbNb += std::ceil((8 * payloadBytes - 4 * SF + 28 + 16 - 20 * 0) / (4 * (SF - 2 * 0))) * (CR + 4);
    if (payloadSymbNb < 8)
        payloadSymbNb = 8;

    double Tsym = std::pow(2, SF) / BW;
    double Tpreamble = (nPreamble + 4.25) * Tsym;
    double Theader = 0.5 * (8 + payloadSymbNb) * Tsym;
    double Tpayload = 0.5 * (8 + payloadSymbNb) * Tsym;
    double ToA =  Tpreamble + Theader + Tpayload;
    EV << "ToA: " << ToA <<endl;


    //double duty_cycle = 0.01;   // according to Juan observation
    double duty_cycle = ToA/timeToNextPacket;
    int num_devices = getSystemModule()->par("numTerminals").intValue();
    double lambda_traffic = (num_devices * duty_cycle) / t_window;
    double P_no_collision = std::exp(-2.0 * lambda_traffic * ToA);

    EV << "lambda_traffic : " << lambda_traffic  <<endl;
    EV << "num_devices: " << num_devices <<endl;
    EV << "duty_cycle: " << duty_cycle <<endl;
    EV << "timeToNextPacket: " << timeToNextPacket <<endl;

    double final_pred = P_link * P_doppler * P_no_collision;
    EV << "final_pred: " << final_pred <<endl;
    threshold =  par("threshold").doubleValue();
    EV << "threshold: " << threshold <<endl;
    if (final_pred >= threshold){
        EV << "<Positive transmit analytical"<<endl;
        return 1;
    }else{
        return 0;
    }

}

//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "CustomReceiver.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Nodes/_10_Terminal/_40_Application/Ter_App.h"

#include <random>

using namespace mac;
using namespace channel;
using namespace routing;

namespace transceiver {

Define_Module(CustomReceiver);

CustomReceiver::CustomReceiver() :
    snirThreshold(NaN),
    satIndex(-1),
    numCollisions(0),
    rcvBelowSensitivity(0),
    numCN0Failures(0)
{
}

void CustomReceiver::initialize(int stage)
{    
    if (stage == INITSTAGE_LOCAL)
    {
        snirThreshold = math::dB2fraction(par("snirThreshold"));
        
        // Initialize center frequency and bandwidth from parameters
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));
        
        // Check if this receiver is in a gateway (satellite) or terminal
        const char* parentClassName = getParentModule()->getClassName();
        if(strcmp(parentClassName, "radio::Sat_Dsl_Phy_LoRaRadio") == 0)
            iAmGateway = true;
        else
            iAmGateway = false;
        
        // Load noise floor parameter
        noiseFloor_dBm = par("noiseFloor_dBm").doubleValue();

        // Load relative interference map file if specified
        relativeMapFile = par("relativeInterferenceMapFile").stdstringValue();
        if (!relativeMapFile.empty()) {
            try {
                relativeMapI0 = CSVReader::loadRelativeMap(relativeMapFile);
                EV << "Loaded relative interference map from: " << relativeMapFile << endl;
            } catch (const std::exception& e) {
                throw cRuntimeError("Failed to load relative interference map: %s", e.what());
            }
        } else {
            EV << "No relative interference map file specified, using default behavior" << endl;
        }
        
        // Load C/N0 reception map file if specified
        cn0MapFile = par("cn0MapFile").stdstringValue();
        if (!cn0MapFile.empty()) {
            try {
                cn0ReceptionMap = CSVReader::loadCN0ReceptionMap(cn0MapFile);
                EV << "Loaded C/N0 reception map from: " << cn0MapFile << endl;
            } catch (const std::exception& e) {
                throw cRuntimeError("Failed to load C/N0 map: %s", e.what());
            }
        } else {
            EV << "No C/N0 map file specified, using default behavior" << endl;
        }
        
        receptionCollisionSignal = registerSignal("receptionCollision");
        belowSensitivitySignal = registerSignal("belowSensitivity");

        // Initialize vector recorder for received power
        receivedPowerVector = new cOutVector("Received Power (dBm)");
        receivedCN0Vector = new cOutVector("Received C/N0 (dB)");
        expectedCN0Vector = new cOutVector("Expected C/N0 (dB)");
        relativeInterferenceVector = new cOutVector("Relative Interference Level (dB)");
        positionWhenInterferenceIsComputedVector = new cOutVector("Satellite Position when Interference Computed");
    }
    
    if (stage == INITSTAGE_LAST && iAmGateway)
        satIndex = getParentModule()->par("satIndex");
}

void CustomReceiver::finish()
{
    recordScalar("numCollisions", numCollisions);
    recordScalar("rcvBelowSensitivity", rcvBelowSensitivity);
    recordScalar("numCN0Failures", numCN0Failures);

    // Clean up vector recorder
    delete receivedPowerVector;
    delete receivedCN0Vector;
    delete expectedCN0Vector;
    delete relativeInterferenceVector;
    delete positionWhenInterferenceIsComputedVector;
}

bool CustomReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    // Gateway can receive any transmission
    // Terminal must match frequency parameters
    const CustomTransmission *customTransmission = check_and_cast<const CustomTransmission *>(transmission);
    
    if (iAmGateway) {
        return true;
    } else {
        // For terminals, check frequency compatibility
        return (customTransmission->getCenterFrequency() == centerFrequency);
    }
}

bool CustomReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    const CustomBandListening *customListening = check_and_cast<const CustomBandListening *>(listening);
    const CustomReception *customReception = check_and_cast<const CustomReception *>(reception);
    
    // Check frequency compatibility (for non-gateway)
    EV << "CustomReceiver::computeIsReceptionPossible - Frequency check: " << (customListening->getCenterFrequency() == customReception->getCenterFrequency() ? "match" : "mismatch") << endl;
    if (!iAmGateway && customListening->getCenterFrequency() != customReception->getCenterFrequency())
    {
        return false;
    }
    
    return true;
}

bool CustomReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    // auto interferingReceptions = interference->getInterferingReceptions();
    
    // if (interferingReceptions->empty()) {
    //     return true;  // No interference
    // }
    
    // const CustomReception *customReception = check_and_cast<const CustomReception *>(reception);
    // simtime_t m_x = (customReception->getStartTime() + customReception->getEndTime()) / 2;
    // simtime_t d_x = (customReception->getEndTime() - customReception->getStartTime()) / 2;
    
    // W signalPower = customReception->getPower();
    // double signalPower_dBm = math::mW2dBmW(signalPower.get() * 1000);
    
    // // Check for temporal and frequency overlap with sufficient interference power
    // for (auto interferingReception : *interferingReceptions) {
    //     const CustomReception *customInterference = check_and_cast<const CustomReception *>(interferingReception);
        
    //     simtime_t m_y = (customInterference->getStartTime() + customInterference->getEndTime()) / 2;
    //     simtime_t d_y = (customInterference->getEndTime() - customInterference->getStartTime()) / 2;
        
    //     bool overlap = (omnetpp::fabs(m_x - m_y) < d_x + d_y);
    //     bool frequencyMatch = (customReception->getCenterFrequency() == customInterference->getCenterFrequency());
        
    //     if (overlap && frequencyMatch) {
    //         W interferencePower = customInterference->getPower();
    //         double interferencePower_dBm = math::mW2dBmW(interferencePower.get() * 1000);
            
    //         // Simple capture effect: if signal is at least 6 dB stronger, no collision
    //         double captureThreshold_dB = 6.0;
    //         if (signalPower_dBm - interferencePower_dBm < captureThreshold_dB) {
    //             // Collision detected
    //             const_cast<CustomReceiver* >(this)->emit(receptionCollisionSignal, true);
    //             const_cast<CustomReceiver* >(this)->numCollisions++;
    //             EV << "Collision detected" << endl;
    //             return false;
    //         }
    //     }
    // }
    
    return true;
}

bool CustomReceiver::decideCN0Reception(double cn0_dB) const
{
    if (cn0ReceptionMap.entries.empty()) {
        // No C/N0 map loaded, always succeed
        std::cout << "No C/N0 map loaded, reception always succeeds." << std::endl;
        return true;
    }
    
    // Get failure rate from C/N0 map
    double failureRate = cn0ReceptionMap.getFailureRate(cn0_dB);
    
    // Stochastic decision
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    double randomValue = dis(gen);
    bool success = (randomValue > failureRate);
    
    EV << "C/N0 = " << cn0_dB << " dB, failure_rate = " << failureRate 
       << ", random = " << randomValue << " -> " << (success ? "SUCCESS" : "FAILURE") << endl;
    
    if (!success) {
        const_cast<CustomReceiver* >(this)->numCN0Failures++;
    }
    else {
        EV << "Reception succeeded based on C/N0 decision." << endl;
        receivedCN0Vector->record(cn0_dB);
    }
    
    return success;
}

const IReceptionDecision *CustomReceiver::computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    EV << "CustomReceiver::computeReceptionDecision" << endl;
    auto isReceptionPossible = computeIsReceptionPossible(listening, reception, part);
    EV << "Computing whether reception is possible: " << (isReceptionPossible ? "possible" : "impossible") << endl;
    auto isReceptionAttempted = isReceptionPossible && computeIsReceptionAttempted(listening, reception, part, interference);
    EV << "Computing whether reception is attempted: " << (isReceptionAttempted ? "attempted" : "not attempted") << endl;
    auto isReceptionSuccessful = isReceptionAttempted && computeIsReceptionSuccessful(listening, reception, part, interference, snir);
    EV << "Computing whether reception is successful: " << (isReceptionSuccessful ? "successful" : "failed") << endl;
    return new ReceptionDecision(reception, part, isReceptionPossible, isReceptionAttempted, isReceptionSuccessful);
}

Packet *CustomReceiver::computeReceivedPacket(const ISnir *snir, bool isReceptionSuccessful) const
{
    auto transmittedPacket = snir->getReception()->getTransmission()->getPacket();
    auto receivedPacket = transmittedPacket->dup();
    
    if (receivedPacket->findTag<CstRoutingTag>()) {
        // receivedPacket->removeTagIfPresent<inet::PacketProtocolTag>();
        receivedPacket->removeTagIfPresent<inet::MacAddressInd>();
        receivedPacket->removeTagIfPresent<inet::SignalPowerReq>();
        receivedPacket->removeTagIfPresent<inet::SnirInd>();
        receivedPacket->removeTagIfPresent<inet::SignalTimeInd>();
        receivedPacket->removeTagIfPresent<inet::ErrorRateInd>();
        receivedPacket->removeTagIfPresent<inet::MacAddressReq>();
    } else {
        receivedPacket->clearTags();
    }
    
    if (!isReceptionSuccessful)
        receivedPacket->setBitError(true);
    
    return receivedPacket;
}

const IReceptionResult *CustomReceiver::computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const
{
    bool isReceptionSuccessful = true;
    for (auto decision : *decisions)
        isReceptionSuccessful &= decision->isReceptionSuccessful();
    
    auto packet = computeReceivedPacket(snir, isReceptionSuccessful);
    
    auto signalPowerInd = packet->addTagIfAbsent<SignalPowerInd>();
    const CustomReception *customReception = check_and_cast<const CustomReception *>(reception);
    W signalPower = customReception->getPower();
    signalPowerInd->setPower(signalPower);
    
    auto snirInd = packet->addTagIfAbsent<SnirInd>();
    snirInd->setMinimumSnir(snir->getMin());
    snirInd->setMaximumSnir(snir->getMax());
    
    auto signalTimeInd = packet->addTagIfAbsent<SignalTimeInd>();
    signalTimeInd->setStartTime(reception->getStartTime());
    signalTimeInd->setEndTime(reception->getEndTime());
    
    auto errorRateInd = packet->addTagIfAbsent<ErrorRateInd>();
    errorRateInd->setPacketErrorRate(errorModel ? errorModel->computePacketErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
    errorRateInd->setBitErrorRate(errorModel ? errorModel->computeBitErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
    errorRateInd->setSymbolErrorRate(errorModel ? errorModel->computeSymbolErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
    
    return new ReceptionResult(reception, decisions, packet);
}

bool CustomReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    const CustomReception *customReception = check_and_cast<const CustomReception *>(reception);
    double bandwidth_Hz = customReception->getBandwidth().get();

    const auto *receiverRadio = listening->getReceiver();
    const auto *receiverMobility = receiverRadio->getAntenna()->getMobility();

    const auto *receiverSatMobility = receiverMobility ? dynamic_cast<const mobility::Sat_Mob_SatelliteMobility *>(receiverMobility) : nullptr;

    double satelliteLatDeg = receiverSatMobility->getLatitude();
    double satelliteLonDeg = receiverSatMobility->getLongitude();

    EV << "CustomReceiver::computeIsReceptionSuccessful - Satellite position (lat, lon): (" << satelliteLatDeg << ", " << satelliteLonDeg << ")" << endl;
    
    // Retrive the received power
    W receivedPower = customReception->getPower();
    double receivedPower_dBm = math::mW2dBmW(receivedPower.get() * 1000);
    EV << "Reception Power: " << receivedPower << " (" << receivedPower_dBm << " dBm)" << endl;

    // Compute system noise power spectral density N0 = k*T_sys
    double k = 1.38064852e-23; // Boltzmann constant
    double T_sys = 578.6; // System temperature in Kelvin
    double noise = k * T_sys;

    double noiseAdjustment = 0; // Adjustment factor to match expected noise floor

    double relativeInterference_dB = relativeMapI0.getValue(satelliteLonDeg, satelliteLatDeg);

    EV << "Base Noise Power Spectral Density N0: " << 10 * log10(noise) << " dBm" << endl;

    double noise_dBm = 10 * log10(noise) + 30 + relativeInterference_dB + noiseAdjustment; // Convert to dBm

    // Record the relative interference level
    relativeInterferenceVector->record(relativeInterference_dB);

    // Encode the lat, lon when interference map is used
    int roundedLatitude = (int)round(satelliteLatDeg);
    int roundedLongitude = (int)round(satelliteLonDeg);
    double positionEncoded = roundedLatitude * 1000.0 + (roundedLongitude + 180.0);
    positionWhenInterferenceIsComputedVector->record(positionEncoded);

    EV << "Relative Interference Level: " << relativeInterference_dB << " dB" << endl;
    EV << "System Noise and interferences Power: " << noise << " W (" << noise_dBm << " dBm)" << endl;

    // Compute C/N0
    double cn0_dB = receivedPower_dBm - noise_dBm;
    
    EV << "Reception C/N0: " << cn0_dB << " dB" << endl;

    expectedCN0Vector->record(cn0_dB);
    
    // Use C/N0-based decision if map is loaded
    if (decideCN0Reception(cn0_dB)){
        receivedPowerVector->record(receivedPower_dBm);
        return true;
    } else {
        return false;
    }
}

const IListening *CustomReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord &startPosition, const Coord &endPosition) const
{
    // Use configured center frequency and bandwidth
    return new CustomBandListening(radio, startTime, endTime, startPosition, endPosition, centerFrequency, bandwidth);
}

const IListeningDecision *CustomReceiver::computeListeningDecision(const IListening *listening, const IInterference *interference) const
{
    const IRadio *receiver = listening->getReceiver();
    const IRadioMedium *radioMedium = receiver->getMedium();
    const IAnalogModel *analogModel = radioMedium->getAnalogModel();
    const INoise *noise = analogModel->computeNoise(listening, interference);
    const ScalarNoise *customNoise = check_and_cast<const ScalarNoise *>(noise);
    W maxPower = customNoise->computeMaxPower(listening->getStartTime(), listening->getEndTime());
    bool isListeningPossible = maxPower >= energyDetection;
    delete noise;
    
    EV_DEBUG << "Computing whether listening is possible: maximum power = " << maxPower 
             << ", energy detection = " << energyDetection 
             << " -> listening is " << (isListeningPossible ? "possible" : "impossible") << endl;
    
    return new ListeningDecision(listening, isListeningPossible);
}

} // namespace transceiver

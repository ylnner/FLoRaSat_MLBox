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

#include "LoRaReceiver.h"

#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarNoise.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
//#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Nodes/_10_Terminal/_40_Application/Ter_App.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"

using namespace mac;
using namespace channel;
using namespace messages;
using namespace routing;

namespace transceiver {

Define_Module(LoRaReceiver);

LoRaReceiver::LoRaReceiver() :
    snirThreshold(NaN),
    minCollisionOverlap(SIMTIME_ZERO)
{
}

void LoRaReceiver::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        snirThreshold = math::dB2fraction(par("snirThreshold"));
        // Check if this receiver is in a gateway (satellite) or terminal
        const char* parentClassName = getParentModule()->getClassName();
        if(strcmp(parentClassName, "radio::Sat_Dsl_Phy_LoRaRadio") == 0)
            iAmGateway = true;
        else
            iAmGateway = false;
        alohaChannelModel = par("alohaChannelModel");
        ignoreInterference = par("ignoreInterference").boolValue();
        numCollisions = 0;
        rcvBelowSensitivity = 0;
        LoRaReceptionCollision = registerSignal("LoRaReceptionCollision");
        belowSensitivityReception = registerSignal("belowSensitivityReception");

        // ACHF
        // Logic to enable/disable different collision handling
        ///enableCollisions                = par("enableCollisions").boolValue();
        enableCollisionOverlap          = par("enableCollisionOverlap").boolValue();
    }

    if (stage == INITSTAGE_LAST && iAmGateway)
        satIndex = getParentModule()->par("satIndex");
}

void LoRaReceiver::finish()
{
        recordScalar("numCollisions", numCollisions);
        recordScalar("rcvBelowSensitivity", rcvBelowSensitivity);

}

bool LoRaReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    //here we can check compatibility of LoRaTx parameters (or beeing a gateway)
    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    
    if (iAmGateway) {
        // Gateway can receive any transmission
        return true;
    } else {
        // Terminal must match LoRa parameters with the application
        auto *loRaApp = check_and_cast<Ter_App *>(getParentModule()->getParentModule()->getSubmodule("app"));
        if(loRaTransmission->getLoRaCF() == loRaApp->loRaCF && loRaTransmission->getLoRaBW() == loRaApp->loRaBW && loRaTransmission->getLoRaSF() == loRaApp->loRaSF)
            return true;
        else
            return false;
    }
}

bool LoRaReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    //here we can check compatibility of LoRaTx parameters (or beeing a gateway) and reception above sensitivity level
    const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);

    //auto *test = getParentModule();
    //const LoRaMac *loRaMac = check_and_cast<const LoRaMac *>(test->getParentModule()->par("mac"));
    if (iAmGateway == false && (loRaListening->getLoRaCF() != loRaReception->getLoRaCF() || loRaListening->getLoRaBW() != loRaReception->getLoRaBW() || loRaListening->getLoRaSF() != loRaReception->getLoRaSF()))
    {
        return false;
    }
    else
    {
        W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
        W sensitivity = getSensitivity(loRaReception);
        EV << "LoRaReceiver::computeIsReceptionPossible" <<endl;
        EV << "minReceptionPower: " << minReceptionPower <<endl;
        double minReceptionPower_dBm = math::mW2dBmW(double(minReceptionPower.get()*1000));
        EV << "minReceptionPower: " << minReceptionPower_dBm << " dBm "<<endl;

        EV << "sensitivity: " << sensitivity <<endl;
        double sensitivity_dBm = math::mW2dBmW(double(sensitivity.get()*1000));
        bool isReceptionPossible = minReceptionPower >= sensitivity;

        EV << "Receiver sensitivity is " << sensitivity << endl;
        EV << "Received sensitivity is " << sensitivity_dBm << " dBm" << endl;
        EV << "Computing whether reception is possible: minimum reception power = " << minReceptionPower << ", sensitivity = "
                << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;

        if(isReceptionPossible == false) {
            const_cast<LoRaReceiver* >(this)->emit(belowSensitivityReception, satIndex);
            const_cast<LoRaReceiver* >(this)->rcvBelowSensitivity++;
        }

        return isReceptionPossible;
    }
}

bool LoRaReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    EV << "LoRaReceiver::computeIsReceptionAttempted" <<endl;
    if(isPacketCollided(reception, part, interference))
    {
        EV << "inside if isPacketCollided" <<endl;
        auto packet = reception->getTransmission()->getPacket();
        const auto &chunk = packet->peekAtFront<FieldsChunk>();

        auto loraMac = dynamicPtrCast<const Base_MacFrame>(chunk);
        auto loraPreamble = dynamicPtrCast<const LoRaPhyPreamble>(chunk);
        MacAddress rec;
        if (loraPreamble)
            rec = loraPreamble->getReceiverAddress();
        else if (loraMac)
            rec = loraMac->getReceiverAddress();

        if (iAmGateway == false)
        {
            auto *macLayer = check_and_cast<Base_MacProtocol *>(getParentModule()->getParentModule()->getSubmodule("mac"));
            if (rec == macLayer->getAddress()) {
                const_cast<LoRaReceiver* >(this)->numCollisions++;
            }
        }
        else
        {
            EV << getParentModule()->getName() <<endl;
            //auto *gwMacLayer = check_and_cast<Base_MacProtocol *>(getParentModule()->getParentModule()->getSubmodule("mac"));
            auto *gwMacLayer = check_and_cast<Base_MacProtocol *>(getParentModule()->getParentModule()->getParentModule()->getSubmodule("dlkDSL")->getSubmodule("mac"));
            EV << "GW: Extracted macFrame = " << rec << ", node address = " << gwMacLayer->getAddress() << std::endl;
            if (rec == MacAddress::BROADCAST_ADDRESS) {
                const_cast<LoRaReceiver* >(this)->numCollisions++;
            }
        }
        return false;
    }
    else
        return true;
}

bool LoRaReceiver::isPacketCollided(const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    // If interference should be ignored, always return false (no collision)
    if (ignoreInterference == 1){
        EV_INFO << "Ignoring interference" << endl;
        return false;
    }

    //auto radio = reception->getReceiver();
    //auto radioMedium = radio->getMedium();
    //EV << "interference: " << interference <<endl;
    auto interferingReceptions = interference->getInterferingReceptions();

    //EV << "interferingReceptions: " << interferingReceptions <<endl;

    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);
    simtime_t m_x = (loRaReception->getStartTime() + loRaReception->getEndTime())/2;
    simtime_t d_x = (loRaReception->getEndTime() - loRaReception->getStartTime())/2;
    EV << "Transmission time is " << 2*d_x << "s" << endl;

    double P_threshold = 6;
    W signalRSSI_w = loRaReception->getPower();
    double signalRSSI_mw = signalRSSI_w.get()*1000;
    double signalRSSI_dBm = math::mW2dBmW(signalRSSI_mw);
    EV << "Received signal RSSI is " << signalRSSI_mw << " mw" << endl;
    EV << "Received signal RSSI is " << signalRSSI_dBm << " dBm" << endl;

    int receptionSF = loRaReception->getLoRaSF();
    for (auto interferingReception : *interferingReceptions) {
        EV << "Inside loop, clean"<<endl;
        bool overlap = false;
        bool frequencyCollision = false;
        bool captureEffect = false;
        bool timingCollision = false; //Collision is acceptable in first part of preamble
        const LoRaReception *loRaInterference = check_and_cast<const LoRaReception *>(interferingReception);

        simtime_t m_y = (loRaInterference->getStartTime() + loRaInterference->getEndTime())/2;
        simtime_t d_y = (loRaInterference->getEndTime() - loRaInterference->getStartTime())/2;
        if(omnetpp::fabs(m_x - m_y) < d_x + d_y)
            overlap = true;

        if(loRaReception->getLoRaCF() == loRaInterference->getLoRaCF())
            frequencyCollision = true;

        W interferenceRSSI_w = loRaInterference->getPower();
        double interferenceRSSI_mw = interferenceRSSI_w.get()*1000;
        double interferenceRSSI_dBm = math::mW2dBmW(interferenceRSSI_mw);
        int interferenceSF = loRaInterference->getLoRaSF();

        /* If difference in power between two signals is greater than threshold, no collision*/
        if(signalRSSI_dBm - interferenceRSSI_dBm >= nonOrthDelta[receptionSF-7][interferenceSF-7])
            captureEffect = true;


        EV << "[MSDEBUG] Received packet at SF: " << receptionSF << " with power " << signalRSSI_dBm << endl;
        EV << "[MSDEBUG] Received interference at SF: " << interferenceSF << " with power " << interferenceRSSI_dBm << endl;
        EV << "[MSDEBUG] Acceptable diff is equal " << nonOrthDelta[receptionSF-7][interferenceSF-7] << endl;
        EV << "[MSDEBUG] Diff is equal " << signalRSSI_dBm - interferenceRSSI_dBm << endl;
        if (captureEffect)
            EV << "[MSDEBUG] Packet is not discarded" << endl;
        else
            EV << "[MSDEBUG] Packet is discarded" << endl;

        // If last 6 symbols of preamble are received, no collision
        // from the paper "Do Lora LPWAN networks scale?"
        double nPreamble = 8;
        simtime_t Tsym = (pow(2, loRaReception->getLoRaSF()))/(loRaReception->getLoRaBW().get()/1000)/1000;
        simtime_t csBegin = loRaReception->getPreambleStartTime() + Tsym * (nPreamble - 6);
        if(csBegin < loRaInterference->getEndTime())
            timingCollision = true;

        if (overlap && frequencyCollision)
        {
            if(alohaChannelModel)
            {
                if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE))
                    const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                return true;
            }

            else
            {
                if(!captureEffect && timingCollision)
                {
                    if(iAmGateway && (part == IRadioSignal::SIGNAL_PART_DATA || part == IRadioSignal::SIGNAL_PART_WHOLE))
                        const_cast<LoRaReceiver* >(this)->emit(LoRaReceptionCollision, true);
                    return true;
                }
            }

        }
    }
    return false;
}

const IReceptionDecision *LoRaReceiver::computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    auto isReceptionPossible = computeIsReceptionPossible(listening, reception, part);
    auto isReceptionAttempted = isReceptionPossible && computeIsReceptionAttempted(listening, reception, part, interference);
    auto isReceptionSuccessful = isReceptionAttempted && computeIsReceptionSuccessful(listening, reception, part, interference, snir);
    return new ReceptionDecision(reception, part, isReceptionPossible, isReceptionAttempted, isReceptionSuccessful);
}

Packet *LoRaReceiver::computeReceivedPacket(const ISnir *snir, bool isReceptionSuccessful) const
{
    auto transmittedPacket = snir->getReception()->getTransmission()->getPacket();
    auto receivedPacket = transmittedPacket->dup();

    if(receivedPacket->findTag<CstRoutingTag>()){
        receivedPacket->removeTagIfPresent<inet::PacketProtocolTag>();
        receivedPacket->removeTagIfPresent<inet::MacAddressInd>();
        receivedPacket->removeTagIfPresent<inet::SignalPowerReq>();
        receivedPacket->removeTagIfPresent<inet::SnirInd>();
        receivedPacket->removeTagIfPresent<inet::SignalTimeInd>();
        receivedPacket->removeTagIfPresent<inet::ErrorRateInd>();
        receivedPacket->removeTagIfPresent<inet::MacAddressReq>();
        receivedPacket->removeTagIfPresent<inet::PacketProtocolTag>();
        receivedPacket->removeTagIfPresent<inet::SignalPowerReq>();
    }else{
        receivedPacket->clearTags();
    }

    //receivedPacket->removeTag<>();
    //INFO (LoRaMedium)RoutingGsToLn.LoRaMedium: i: 6 -- inet::PacketProtocolTag-
    //INFO (LoRaMedium)RoutingGsToLn.LoRaMedium: i: 7 -- inet::MacAddressInd-
    //INFO (LoRaMedium)RoutingGsToLn.LoRaMedium: i: 8 -- inet::SignalPowerReq-


//    receivedPacket->addTag<PacketProtocolTag>()->setProtocol(transmittedPacket->getTag<PacketProtocolTag>()->getProtocol());
    if (!isReceptionSuccessful)
        receivedPacket->setBitError(true);
    return receivedPacket;
}

const IReceptionResult *LoRaReceiver::computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const
{
    bool isReceptionSuccessful = true;
    for (auto decision : *decisions)
        isReceptionSuccessful &= decision->isReceptionSuccessful();
    auto packet = computeReceivedPacket(snir, isReceptionSuccessful);

    auto signalPowerInd = packet->addTagIfAbsent<SignalPowerInd>();
    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(reception);
    W signalRSSI_w = loRaReception->getPower();
    signalPowerInd->setPower(signalRSSI_w);

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

bool LoRaReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    const LoRaReception *loraReception = check_and_cast<const LoRaReception *>(reception);
    auto interferingReceptions = interference->getInterferingReceptions();

    EV_INFO << "LoRaReceiver::computeIsReceptionSuccessful - Checking for collisions with " << interferingReceptions->size() << " interfering receptions" << endl;
    EV_INFO << ", Enable collision overlap: " << (enableCollisionOverlap ? "YES" : "NO") << endl;
    //<< "Enable collisions: " << (enableCollisions ? "YES" : "NO")


    /*
    // Record start time of reception for validation
    receptionStartTimeVector->record(customReception->getStartTime().dbl());

    // ---- Doppler rate computation ----
    // 1) Doppler shift at the START of reception (already stored in the reception object)
    const CustomTransmission *customTransmission = check_and_cast<const CustomTransmission *>(customReception->getTransmission());
    Hz originalFreq       = customTransmission->getActualTransmissionFrequency();
    Hz dopplerFreqAtStart = customReception->getDopplerShiftedFrequency();
    double dopplerShift_start = dopplerFreqAtStart.get() - originalFreq.get(); // Hz

    // 2) Doppler shift at the END of reception: call the analog model again (satellite has moved)
    const IRadio         *receiverRadio  = listening->getReceiver();
    const IRadioMedium   *radioMedium    = receiverRadio->getMedium();
    const channel::CustomPHYAnalogModel *analogModel =
        check_and_cast<const channel::CustomPHYAnalogModel *>(radioMedium->getAnalogModel());
    Hz dopplerFreqAtEnd   = analogModel->computeDopplerFrequency(receiverRadio, customTransmission);
    double dopplerShift_end = dopplerFreqAtEnd.get() - originalFreq.get(); // Hz
    endDopplerVector->record(dopplerFreqAtEnd.get());

    // 3) Doppler rate = Δ(Doppler shift) / packet duration  [Hz/s]
    double packetDuration = (customReception->getEndTime() - customReception->getStartTime()).dbl();
    double dopplerRate    = (packetDuration > 0.0) ? (dopplerShift_end - dopplerShift_start) / packetDuration : 0.0;

    EV_INFO << "Doppler shift at start: " << dopplerShift_start / 1e3 << " kHz"
            << ", at end: "  << dopplerShift_end   / 1e3 << " kHz"
            << ", packet duration: " << packetDuration << " s"
            << ", Doppler rate: " << dopplerRate << " Hz/s" << endl;
    dopplerRateVector->record(dopplerRate);
    // ---- End Doppler rate computation ----
     */

    /*
    // Always recompute C/N0 at reception decision time.
    double cn0_dB = computeCn0Value(listening, customReception);
    const_cast<CustomReception *>(customReception)->setCn0_dB(cn0_dB);
    EV_INFO << "Reception C/N0: " << cn0_dB << " dB" << endl;

    int overlappingInterferers = countOverlappingInterferers(customReception, interference);
    int concurrentTransmissions = overlappingInterferers + 1; // include the current packet
    double normalizedLoad = concurrentTransmissionsToNormalizedLoad(concurrentTransmissions);
    double perFromLoad = computePerFromLoadAndCn0(normalizedLoad, cn0_dB);
    auto *mutableReception = const_cast<CustomReception *>(customReception);
    mutableReception->setInterferingPacketCount(overlappingInterferers);
    mutableReception->setNormalizedLoad(normalizedLoad);
    mutableReception->setPerFromLoad(perFromLoad);
    normalizedLoadVector->record(normalizedLoad);
    perFromLoadVector->record(perFromLoad);

    EV_INFO << "Interfering packets during reception: " << overlappingInterferers
            << ", concurrent transmissions: " << concurrentTransmissions
            << ", normalized load: " << normalizedLoad
            << ", PER(load,C/N0): " << perFromLoad
            << " (mode=" << (useLoadCn0PerCsv ? "csv" : "average") << ")" << endl;

    // If collision handling is disabled, keep C/N0-only behavior.
    if (!enableCollisions) {
        expectedCN0Vector->record(cn0_dB);
        if (decideCN0Reception(cn0_dB)) {
            W receivedPower = customReception->getPower();
            double receivedPower_dBm = math::mW2dBmW(receivedPower.get() * 1000);
            receivedPowerVector->record(receivedPower_dBm);
            return true;
        }
        return false;
    }

    // Independent per-packet collision decision from PER(load,C/N0).
    // This does not propagate collision status to any concurrent packet.
    if (!ComputeSuccessUsingPerFromLoad(perFromLoad)) {
        const_cast<CustomReceiver *>(this)->emit(receptionCollisionSignal, true);
        const_cast<CustomReceiver *>(this)->numCollisions++;
        collisionEventVector->record(1);
        return false;
    }
    */

    // If collision overlap detection is enabled, check for temporal overlap with interferers
    if (enableCollisionOverlap && detectCollisionOverlap(loraReception, interference)) {
        //const_cast<CustomReceiver *>(this)->emit(receptionCollisionSignal, true);
        //const_cast<CustomReceiver *>(this)->numCollisions++;
        //collisionEventVector->record(1);

        return false;
    }
    //collisionEventVector->record(0);
    return true; // No collision detected, reception successful



    //return true;
    //we don't check the SINR level, it is done in collision checking by P_threshold level evaluation
}

bool LoRaReceiver::detectCollisionOverlap(const LoRaReception *loraReception, const IInterference *interference) const
{
    EV << "LoRaReceiver::detectCollisionOverlap "<<endl;
    auto interferingReceptions = interference->getInterferingReceptions();
    if (interferingReceptions->empty()) {
        return false;
    }

    simtime_t start_x = loraReception->getStartTime();
    simtime_t end_x   = loraReception->getEndTime();
    Hz freq_x         = loraReception->getDopplerShiftedFrequency();

    EV << "envFalgs loraReception: " << loraReception->getTransmission()->getTransmitter()->getCompleteStringRepresentation(evFlags)<<endl;

    // Check for temporal OR frequency overlap
    for (auto interferingReception : *interferingReceptions) {
        const LoRaReception *interference = check_and_cast<const LoRaReception *>(interferingReception);

        EV << "envFalgs interference: " << interference->getTransmission()->getTransmitter()->getCompleteStringRepresentation(evFlags)<<endl;

        simtime_t start_y = interference->getStartTime();
        simtime_t end_y = interference->getEndTime();
        Hz freq_y = interference->getDopplerShiftedFrequency();

        // Temporal overlap check
        simtime_t overlapDuration = std::min(end_x, end_y) - std::max(start_x, start_y);
        bool temporalOverlap = overlapDuration > SIMTIME_ZERO;
        bool overlapExceedsThreshold = overlapDuration >= minCollisionOverlap;


        EV << "loraReception.freq: " << freq_x.get() <<endl;
        EV << "interference.freq: " << freq_y.get() <<endl;
        // Frequency overlap check: separated by less than 1200 Hz
        double freqSeparation = std::abs(freq_x.get() - freq_y.get());
        bool frequencyOverlap = freqSeparation < 1200.0;

        EV_INFO << "Temporal overlap: " << (temporalOverlap ? "YES" : "NO") << ", overlap duration: " << overlapDuration << endl;
        EV_INFO << "Overlap duration exceeds threshold: " << (overlapExceedsThreshold ? "YES" : "NO") << ", threshold: " << minCollisionOverlap << endl;
        EV_INFO << "Frequency overlap: " << (frequencyOverlap ? "YES" : "NO") << ", frequency separation: " << freqSeparation << " Hz" << endl;
        EV_INFO << "Collision condition met: " << (temporalOverlap && overlapExceedsThreshold && frequencyOverlap ? "YES" : "NO") << endl;

        // Collision detected due to temporal or frequency overlap
        if (temporalOverlap && overlapExceedsThreshold && frequencyOverlap) {
            EV_INFO << "Collision detected: temporal overlap AND frequency overlap" << endl;
            return true;
        }
    }
    EV_INFO << "No collision detected based on temporal and frequency overlap checks." << endl;
    return false;
}



const IListening *LoRaReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord &startPosition, const Coord &endPosition) const
{
    if(iAmGateway == false)
    {
        // EV << "Here"<<endl;
        //auto node = getContainingNode(this);

        cModule *m = this->getParentModule();
        while (m && !m->getSubmodule("app"))
            m = m->getParentModule();

        if (m)
            EV << "getContainingNode() returned: " << m->getFullPath() << endl;
        else
            EV << "getContainingNode() returned NULL" << endl;

        auto loRaApp = check_and_cast<Ter_App *>(m->getSubmodule("app"));
        return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition, loRaApp->loRaCF, loRaApp->loRaBW, loRaApp->loRaSF);
    }
    else return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition, LoRaCF, LoRaBW, LoRaSF);
}

const IListeningDecision *LoRaReceiver::computeListeningDecision(const IListening *listening, const IInterference *interference) const
{
    const IRadio *receiver = listening->getReceiver();
    const IRadioMedium *radioMedium = receiver->getMedium();
    const IAnalogModel *analogModel = radioMedium->getAnalogModel();
    const INoise *noise = analogModel->computeNoise(listening, interference);
    const ScalarNoise *loRaNoise = check_and_cast<const ScalarNoise *>(noise);
    W maxPower = loRaNoise->computeMaxPower(listening->getStartTime(), listening->getEndTime());
    bool isListeningPossible = maxPower >= energyDetection;
    delete noise;
    EV_DEBUG << "Computing whether listening is possible: maximum power = " << maxPower << ", energy detection = " << energyDetection << " -> listening is " << (isListeningPossible ? "possible" : "impossible") << endl;
    return new ListeningDecision(listening, isListeningPossible);
}

W LoRaReceiver::getSensitivity(const LoRaReception *reception) const
{
    //function returns sensitivity -- according to LoRa documentation, it changes with LoRa parameters
    //Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
    W sensitivity = W(math::dBmW2mW(-126.5) / 1000);
    if(reception->getLoRaSF() == 6)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-121) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-118) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-111) / 1000);
    }

    if (reception->getLoRaSF() == 7)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-124) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-116) / 1000);
    }

    if(reception->getLoRaSF() == 8)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-127) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-119) / 1000);
    }
    if(reception->getLoRaSF() == 9)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
    }
    if(reception->getLoRaSF() == 10)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-133) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
    }
    if(reception->getLoRaSF() == 11)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-132) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
    }
    if(reception->getLoRaSF() == 12)
    {
        if(reception->getLoRaBW() == Hz(125000)) sensitivity = W(math::dBmW2mW(-137) / 1000);
        if(reception->getLoRaBW() == Hz(250000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getLoRaBW() == Hz(500000)) sensitivity = W(math::dBmW2mW(-129) / 1000);
    }
    EV << "LoRaReceiver::getSensitivity"<<endl;
    EV << "reception->getLoRaSF(): " << reception->getLoRaSF() <<endl;
    EV << "sensitivity: " << sensitivity <<endl;
    return sensitivity;
}

}

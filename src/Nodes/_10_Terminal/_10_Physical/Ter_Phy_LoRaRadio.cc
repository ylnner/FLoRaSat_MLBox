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

#include "Ter_Phy_LoRaRadio.h"

#include <cmath>
#include <cstring>

#include "Global/Medium/DSL/LoRaMedium.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"

#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"
#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"
#include "Global/Messages/_20_Data_Link/LoRaTagInfo_m.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"

#include "inet/common/Units.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

using namespace mac;
using namespace messages;
using namespace satellite;
using namespace medium;

namespace radio {

Define_Module(Ter_Phy_LoRaRadio);

simsignal_t Ter_Phy_LoRaRadio::minSNIRSignal = cComponent::registerSignal("minSNIR");
simsignal_t Ter_Phy_LoRaRadio::packetErrorRateSignal = cComponent::registerSignal("packetErrorRate");
simsignal_t Ter_Phy_LoRaRadio::bitErrorRateSignal = cComponent::registerSignal("bitErrorRate");
simsignal_t Ter_Phy_LoRaRadio::symbolErrorRateSignal = cComponent::registerSignal("symbolErrorRate");
simsignal_t Ter_Phy_LoRaRadio::droppedPacket = cComponent::registerSignal("droppedPacket");


void Ter_Phy_LoRaRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        iAmGateway = par("iAmGateway").boolValue();
        hasActiveReception = false;
    }
}

std::ostream& Ter_Phy_LoRaRadio::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << static_cast<const cSimpleModule *>(this);
    if (level <= PRINT_LEVEL_TRACE)
        stream << ", antenna = " << printFieldToString(antenna, level + 1, evFlags)
               << ", transmitter = " << printFieldToString(transmitter, level + 1, evFlags)
               << ", receiver = " << printFieldToString(receiver, level + 1, evFlags);
    return stream;
}


const ITransmission *Ter_Phy_LoRaRadio::getTransmissionInProgress() const
{
    if (!transmissionTimer->isScheduled())
        return nullptr;
    else
        return static_cast<WirelessSignal *>(transmissionTimer->getContextPointer())->getTransmission();
}

const ITransmission *Ter_Phy_LoRaRadio::getReceptionInProgress() const
{
    if (receptionTimer == nullptr)
        return nullptr;
    else
        return static_cast<WirelessSignal *>(receptionTimer->getControlInfo())->getTransmission();
}

IRadioSignal::SignalPart Ter_Phy_LoRaRadio::getTransmittedSignalPart() const
{
    return transmittedSignalPart;
}

IRadioSignal::SignalPart Ter_Phy_LoRaRadio::getReceivedSignalPart() const
{
    return receivedSignalPart;
}

void Ter_Phy_LoRaRadio::handleMessageWhenDown(cMessage *message)
{
    if (message->getArrivalGate() == radioIn || isReceptionTimer(message))
        delete message;
    else
        OperationalBase::handleMessageWhenDown(message);
}

void Ter_Phy_LoRaRadio::handleMessageWhenUp(cMessage *message)
{
    if (message->isSelfMessage()) {
        handleSelfMessage(message);
        return;
    }

    auto *arrivalGate = message->getArrivalGate();
    if (arrivalGate == upperLayerIn) {
        if (!message->isPacket()) {
            handleUpperCommand(message);
            delete message;
        }
        else {
            handleUpperPacket(check_and_cast<Packet *>(message));
    }
        return;
    }

    if (arrivalGate == radioIn) {
        if (!message->isPacket()) {
            handleLowerCommand(message);
            delete message;
        }
        else {
            handleSignal(check_and_cast<WirelessSignal *>(message));
        }
        return;
    }

    throw cRuntimeError("Unknown arrival gate '%s'.", arrivalGate->getFullName());
}

void Ter_Phy_LoRaRadio::handleSelfMessage(cMessage *message)
{
    FlatRadioBase::handleSelfMessage(message);
}

void Ter_Phy_LoRaRadio::handleTransmissionTimer(cMessage *message)
{
    switch (static_cast<IRadioSignal::SignalPart>(message->getKind())) {
        case IRadioSignal::SIGNAL_PART_PREAMBLE:
        case IRadioSignal::SIGNAL_PART_HEADER:
            continueTransmission();
            break;
        case IRadioSignal::SIGNAL_PART_WHOLE:
        case IRadioSignal::SIGNAL_PART_DATA:
            endTransmission();
            break;
        default:
            throw cRuntimeError("Unknown transmission timer kind %d", message->getKind());
    }
}

void Ter_Phy_LoRaRadio::handleReceptionTimer(cMessage *message)
{
    switch (static_cast<IRadioSignal::SignalPart>(message->getKind())) {
        case IRadioSignal::SIGNAL_PART_PREAMBLE:
        case IRadioSignal::SIGNAL_PART_HEADER:
            continueReception(message);
            break;
        case IRadioSignal::SIGNAL_PART_WHOLE:
        case IRadioSignal::SIGNAL_PART_DATA:
            endReception(message);
            break;
        default:
            throw cRuntimeError("Unknown reception timer kind %d", message->getKind());
    }
}

void Ter_Phy_LoRaRadio::handleUpperCommand(cMessage *message)
{
    if (message->getKind() == RADIO_C_CONFIGURE) {
        ConfigureRadioCommand *configureCommand = check_and_cast<ConfigureRadioCommand *>(message->getControlInfo());
        if (configureCommand->getRadioMode() != -1)
            setRadioMode((RadioMode)configureCommand->getRadioMode());
    }
    else
        throw cRuntimeError("Unsupported command");
}

void Ter_Phy_LoRaRadio::handleLowerCommand(cMessage *message)
{
    throw cRuntimeError("Unsupported command");
}

void Ter_Phy_LoRaRadio::handleUpperPacket(Packet *packet)
{
    emit(packetReceivedFromUpperSignal, packet);
    if (!isTransmitterMode(radioMode)) {
        EV_ERROR << "Radio not ready for transmission, drop " << packet->getFullName() << endl;
        delete packet;
        return;
    }

    if (packet->findTag<LoRaTag>() != nullptr)
        packet->removeTag<LoRaTag>();

    auto preamble = makeShared<LoRaPhyPreamble>();
    
    // Peek as concrete KiWanFrame type to avoid base class materialization issues
    const auto& frame = packet->peekAtFront<KiWanFrame>();

    preamble->setBandwidth(frame->getLoRaBW());
    preamble->setCenterFrequency(frame->getLoRaCF());
    preamble->setCodeRendundance(frame->getLoRaCR());

    // ACHF
    //loraTag->setPower(W(math::dBmW2mW(loRaTP)));
    preamble->setPower(mW(frame->getLoRaTP())); // Equal to Benoit MIgration
    //preamble->setPower(W(math::dBmW2mW(frame->getLoRaTP())));

    preamble->setSpreadFactor(frame->getLoRaSF());
    preamble->setUseHeader(frame->getLoRaUseHeader());
    preamble->setReceiverAddress(frame->getReceiverAddress());
    preamble->setChunkLength(b(16));

    auto signalPowerReq = packet->addTagIfAbsent<SignalPowerReq>();
    signalPowerReq->setPower(mW(frame->getLoRaTP()));

    packet->insertAtFront(preamble);

    if (transmissionTimer->isScheduled())
        throw cRuntimeError("Received frame from upper layer while already transmitting.");

    const auto part = separateTransmissionParts ? IRadioSignal::SIGNAL_PART_PREAMBLE
                                                : IRadioSignal::SIGNAL_PART_WHOLE;
    startTransmission(packet, part);
}

void Ter_Phy_LoRaRadio::handleSignal(WirelessSignal *radioFrame)
{
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
}

void Ter_Phy_LoRaRadio::startTransmission(Packet *macFrame, IRadioSignal::SignalPart part)
{
    auto signal = createSignal(macFrame);
    auto transmission = signal->getTransmission();

    if (macFrame->findTag<inet::TransmissionTimeTag>() != nullptr) {
    const simtime_t finishTime = transmission->getEndTime();
    const simtime_t duration = finishTime - simTime();
    ASSERT(duration > 0);

        // INET 4.5: TransmissionTimeTag uses appendBitTotalTimes() instead of appendTotalTimes()
        macFrame->getTagForUpdate<inet::TransmissionTimeTag>()->appendBitTotalTimes(duration);
        macFrame->addTagIfAbsent<SendAtTag>()->setSendAt(finishTime);
    }

    transmissionTimer->setKind(part);
    transmissionTimer->setContextPointer(const_cast<WirelessSignal *>(signal));

#ifdef NS3_VALIDATION
    auto *df = dynamic_cast<inet::ieee80211::Ieee80211DataHeader *>(macFrame);
    const char *ac = "NA";
    if (df != nullptr && df->getType() == inet::ieee80211::ST_DATA_WITH_QOS) {
        switch (df->getTid()) {
            case 1: case 2: ac = "AC_BK"; break;
            case 0: case 3: ac = "AC_BE"; break;
            case 4: case 5: ac = "AC_VI"; break;
            case 6: case 7: ac = "AC_VO"; break;
            default: ac = "???"; break;
        }
    }
    const char *lastSeq = strchr(macFrame->getName(), '-');
    if (lastSeq == nullptr)
        lastSeq = "-1";
    else
        lastSeq++;
    std::cout << "TX: node = " << getId() << ", ac = " << ac << ", seq = " << lastSeq << ", start = " << simTime().inUnit(SIMTIME_PS) << ", duration = " << signal->getDuration().inUnit(SIMTIME_PS) << std::endl;
#endif

    scheduleAt(transmission->getEndTime(part), transmissionTimer);
    EV_INFO << "Transmission started: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    updateTransceiverState();
    updateTransceiverPart();
    emit(transmissionStartedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalDepartureStartedSignal, check_and_cast<const cObject *>(transmission));
}

void Ter_Phy_LoRaRadio::continueTransmission()
{
    FlatRadioBase::continueTransmission();
}

void Ter_Phy_LoRaRadio::endTransmission()
{
    FlatRadioBase::endTransmission();

}

void Ter_Phy_LoRaRadio::abortTransmission()
{
    FlatRadioBase::abortTransmission();
}

WirelessSignal *Ter_Phy_LoRaRadio::createSignal(Packet *packet) const
{
    return FlatRadioBase::createSignal(packet);
}

void Ter_Phy_LoRaRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = signal->getArrival();
    auto reception = signal->getReception();
    if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
        auto transmission = signal->getTransmission();
        auto decision = medium->getReceptionDecision(this, signal->getListening(), transmission, part);
        const bool success = decision->isReceptionSuccessful();

        if (success) {
            receptionTimer = timer;
            hasActiveReception = true;
            emit(receptionStartedSignal, check_and_cast<const cObject *>(reception));
        }
        else if (!hasActiveReception) {
            emit(Ter_Phy_LoRaRadio::droppedPacket, 0);
        }

        EV_INFO << "Reception started: " << (success ? "successful" : "unsuccessful")
                << " for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part)
                << " as " << reception << endl;
    }
    else {
        EV_INFO << "Reception started: ignoring " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    }
    timer->setKind(part);
    scheduleAt(arrival->getEndTime(part), timer);
    updateTransceiverState();
    updateTransceiverPart();
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject *>(reception));

}

void Ter_Phy_LoRaRadio::continueReception(cMessage *timer)
{
    FlatRadioBase::continueReception(timer);
}

void Ter_Phy_LoRaRadio::decapsulate(Packet *packet) const
{
    auto tag = packet->addTag<LoRaTag>();
    auto preamble = packet->popAtFront<LoRaPhyPreamble>();

    tag->setBandwidth(preamble->getBandwidth());
    tag->setCenterFrequency(preamble->getCenterFrequency());
    tag->setCodeRendundance(preamble->getCodeRendundance());
    tag->setPower(preamble->getPower());
    tag->setSpreadFactor(preamble->getSpreadFactor());
    tag->setUseHeader(preamble->getUseHeader());
}

void Ter_Phy_LoRaRadio::endReception(cMessage *timer)
{
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = signal->getArrival();
    auto reception = signal->getReception();

    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
        auto transmission = signal->getTransmission();

        auto decision = medium->getReceptionDecision(this, signal->getListening(), transmission, part);
        const bool success = decision->isReceptionSuccessful();
        EV_INFO << "Reception ended: " << (success ? "successfully" : "unsuccessfully") << " for "
                << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        auto macFrame = medium->receivePacket(this, signal);
        take(macFrame);
        decapsulate(macFrame);

        if (success)
            sendUp(macFrame);

        else {
            emit(Ter_Phy_LoRaRadio::droppedPacket, 0);
            delete macFrame;
        }
        receptionTimer = nullptr;
        hasActiveReception = false;
        emit(receptionEndedSignal, check_and_cast<const cObject *>(reception));
    }
    else
        EV_INFO << "Reception ended: \x1b[1mignoring\x1b[0m " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    updateTransceiverState();
    updateTransceiverPart();
    delete timer;
    // TODO: move to radio medium
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
}

void Ter_Phy_LoRaRadio::abortReception(cMessage *timer)
{
    if (timer == receptionTimer)
        hasActiveReception = false;
    FlatRadioBase::abortReception(timer);
}

void Ter_Phy_LoRaRadio::captureReception(cMessage *timer)
{
    // TODO: this would be called when the receiver switches to a stronger signal while receiving a weaker one
    throw cRuntimeError("Not yet implemented");
}

void Ter_Phy_LoRaRadio::sendUp(Packet *macFrame)
{
    auto signalPowerInd = macFrame->findTag<SignalPowerInd>();
    if (signalPowerInd == nullptr)
        throw cRuntimeError("signal Power indication not present");
    auto snirInd =  macFrame->findTag<SnirInd>();
    if (snirInd == nullptr)
        throw cRuntimeError("snir indication not present");

    auto errorTag = macFrame->findTag<ErrorRateInd>();

    emit(minSNIRSignal, snirInd->getMinimumSnir());
    if (errorTag && !std::isnan(errorTag->getPacketErrorRate()))
        emit(packetErrorRateSignal, errorTag->getPacketErrorRate());
    if (errorTag && !std::isnan(errorTag->getBitErrorRate()))
        emit(bitErrorRateSignal, errorTag->getBitErrorRate());
    if (errorTag && !std::isnan(errorTag->getSymbolErrorRate()))
        emit(symbolErrorRateSignal, errorTag->getSymbolErrorRate());
    EV_INFO << "Sending up " << macFrame << endl;

    FlatRadioBase::sendUp(macFrame);
}


double Ter_Phy_LoRaRadio::getCurrentTxPower() const
{
    return currentTxPower;
}

void Ter_Phy_LoRaRadio::setCurrentTxPower(double txPower)
{
    currentTxPower = txPower;
}

} // namespace radio

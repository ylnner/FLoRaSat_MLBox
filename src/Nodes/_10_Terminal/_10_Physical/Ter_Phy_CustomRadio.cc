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

#include "Ter_Phy_CustomRadio.h"

#include <cmath>
#include <cstring>

#include "Global/Medium/DSL/CustomMedium.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"

#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"
#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"
#include "Global/Messages/_20_Data_Link/LoRaTagInfo_m.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"

#include "inet/common/Units.h"
#include "inet/common/TimeTag_m.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

using namespace mac;
using namespace messages;
using namespace satellite;
using namespace medium;

namespace radio {

Define_Module(Ter_Phy_CustomRadio);

simsignal_t Ter_Phy_CustomRadio::minSNIRSignal = cComponent::registerSignal("minSNIR");
simsignal_t Ter_Phy_CustomRadio::packetErrorRateSignal = cComponent::registerSignal("packetErrorRate");
simsignal_t Ter_Phy_CustomRadio::bitErrorRateSignal = cComponent::registerSignal("bitErrorRate");
simsignal_t Ter_Phy_CustomRadio::symbolErrorRateSignal = cComponent::registerSignal("symbolErrorRate");
simsignal_t Ter_Phy_CustomRadio::droppedPacket = cComponent::registerSignal("droppedPacket");

void Ter_Phy_CustomRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        iAmGateway = par("iAmGateway").boolValue();
        hasActiveReception = false;
    }
}

std::ostream& Ter_Phy_CustomRadio::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << static_cast<const cSimpleModule *>(this);
    if (level <= PRINT_LEVEL_TRACE)
        stream << ", antenna = " << printFieldToString(antenna, level + 1, evFlags)
               << ", transmitter = " << printFieldToString(transmitter, level + 1, evFlags)
               << ", receiver = " << printFieldToString(receiver, level + 1, evFlags);
    return stream;
}

const ITransmission *Ter_Phy_CustomRadio::getTransmissionInProgress() const
{
    if (!transmissionTimer->isScheduled())
        return nullptr;
    else
        return static_cast<WirelessSignal *>(transmissionTimer->getContextPointer())->getTransmission();
}

const ITransmission *Ter_Phy_CustomRadio::getReceptionInProgress() const
{
    if (receptionTimer == nullptr)
        return nullptr;
    else
        return static_cast<WirelessSignal *>(receptionTimer->getControlInfo())->getTransmission();
}

IRadioSignal::SignalPart Ter_Phy_CustomRadio::getTransmittedSignalPart() const
{
    return transmittedSignalPart;
}

IRadioSignal::SignalPart Ter_Phy_CustomRadio::getReceivedSignalPart() const
{
    return receivedSignalPart;
}

void Ter_Phy_CustomRadio::handleMessageWhenDown(cMessage *message)
{
    if (message->getArrivalGate() == radioIn || isReceptionTimer(message))
        delete message;
    else
        OperationalBase::handleMessageWhenDown(message);
}

void Ter_Phy_CustomRadio::handleMessageWhenUp(cMessage *message)
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

void Ter_Phy_CustomRadio::handleSelfMessage(cMessage *message)
{
    FlatRadioBase::handleSelfMessage(message);
}

void Ter_Phy_CustomRadio::handleTransmissionTimer(cMessage *message)
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

void Ter_Phy_CustomRadio::handleReceptionTimer(cMessage *message)
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

void Ter_Phy_CustomRadio::handleUpperCommand(cMessage *message)
{
    if (message->getKind() == RADIO_C_CONFIGURE) {
        ConfigureRadioCommand *configureCommand = check_and_cast<ConfigureRadioCommand *>(message->getControlInfo());
        if (configureCommand->getRadioMode() != -1)
            setRadioMode((RadioMode)configureCommand->getRadioMode());
    }
    else
        throw cRuntimeError("Unsupported command");
}

void Ter_Phy_CustomRadio::handleLowerCommand(cMessage *message)
{
    throw cRuntimeError("Unsupported command");
}

void Ter_Phy_CustomRadio::handleUpperPacket(Packet *packet)
{
    emit(packetReceivedFromUpperSignal, packet);
    if (!isTransmitterMode(radioMode)) {
        EV_ERROR << "Radio not ready for transmission, drop " << packet->getFullName() << endl;
        delete packet;
        return;
    }

    // For custom radio, we don't need to add/modify preamble with LoRa parameters
    // The CustomTransmitter will handle transmission details
    
    const auto signalPowerReq = packet->addTagIfAbsent<SignalPowerReq>();
    signalPowerReq->setPower(mW(501.2)); // Default power

    if (transmissionTimer->isScheduled())
        throw cRuntimeError("Received frame from upper layer while already transmitting.");

    const auto part = separateTransmissionParts ? IRadioSignal::SIGNAL_PART_PREAMBLE
                                                : IRadioSignal::SIGNAL_PART_WHOLE;
    startTransmission(packet, part);
}

void Ter_Phy_CustomRadio::handleSignal(WirelessSignal *radioFrame)
{
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
}

void Ter_Phy_CustomRadio::startTransmission(Packet *macFrame, IRadioSignal::SignalPart part)
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

    scheduleAt(transmission->getEndTime(part), transmissionTimer);
    EV_INFO << "Transmission started: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    updateTransceiverState();
    updateTransceiverPart();
    emit(transmissionStartedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalDepartureStartedSignal, check_and_cast<const cObject *>(transmission));
}

void Ter_Phy_CustomRadio::continueTransmission()
{
    FlatRadioBase::continueTransmission();
}

void Ter_Phy_CustomRadio::endTransmission()
{
    FlatRadioBase::endTransmission();
}

void Ter_Phy_CustomRadio::abortTransmission()
{
    FlatRadioBase::abortTransmission();
}

WirelessSignal *Ter_Phy_CustomRadio::createSignal(Packet *packet) const
{
    return FlatRadioBase::createSignal(packet);
}

void Ter_Phy_CustomRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
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
            emit(Ter_Phy_CustomRadio::droppedPacket, 0);
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
    check_and_cast<CustomMedium *>(medium.get())->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject *>(reception));
}

void Ter_Phy_CustomRadio::continueReception(cMessage *timer)
{
    FlatRadioBase::continueReception(timer);
}

void Ter_Phy_CustomRadio::decapsulate(Packet *packet) const
{
    // For custom radio, we don't have LoRa-specific tags to extract
    // Keep packet as-is
}

void Ter_Phy_CustomRadio::endReception(cMessage *timer)
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
            emit(Ter_Phy_CustomRadio::droppedPacket, 0);
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
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
}

void Ter_Phy_CustomRadio::abortReception(cMessage *timer)
{
    if (timer == receptionTimer)
        hasActiveReception = false;
    FlatRadioBase::abortReception(timer);
}

void Ter_Phy_CustomRadio::captureReception(cMessage *timer)
{
    throw cRuntimeError("Not yet implemented");
}

void Ter_Phy_CustomRadio::sendUp(Packet *macFrame)
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

double Ter_Phy_CustomRadio::getCurrentTxPower() const
{
    return currentTxPower;
}

void Ter_Phy_CustomRadio::setCurrentTxPower(double txPower)
{
    currentTxPower = txPower;
}

} // namespace radio

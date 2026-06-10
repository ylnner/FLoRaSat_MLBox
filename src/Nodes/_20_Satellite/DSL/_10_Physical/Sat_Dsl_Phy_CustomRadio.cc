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

#include "Sat_Dsl_Phy_CustomRadio.h"

#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"
#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"

using namespace mac;
using namespace satellite;
using namespace medium;

namespace radio {

Define_Module(Sat_Dsl_Phy_CustomRadio);

void Sat_Dsl_Phy_CustomRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    iAmGateway = par("iAmGateway").boolValue();
    satIndex = par("satIndex");
    if (stage == INITSTAGE_LAST) {
        setRadioMode(RADIO_MODE_TRANSCEIVER);
        Sat_Dsl_Phy_CustomRadioReceptionStarted = registerSignal("Sat_Dsl_Phy_CustomRadioReceptionStarted");
        Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect = registerSignal("Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect");
        Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring = registerSignal("Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring");
        Sat_Dsl_Phy_CustomRadioReceptionStarted_counter = 0;
        Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect_counter = 0;
        iAmTransmiting = false;
    }
}

void Sat_Dsl_Phy_CustomRadio::finish()
{
    FlatRadioBase::finish();
    if (Sat_Dsl_Phy_CustomRadioReceptionStarted_counter > 0)
        recordScalar("DER - Data Extraction Rate", double(Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect_counter)/Sat_Dsl_Phy_CustomRadioReceptionStarted_counter);
    recordScalar("Number of receptions finished with error", countReceptionsFinishedIncorrect);
}

void Sat_Dsl_Phy_CustomRadio::handleSelfMessage(cMessage *message)
{
    EV << "Sat_Dsl_Phy_CustomRadio::handleSelfMessage" << endl;
    if (message == switchTimer){
        handleSwitchTimer(message);
    }
    else if (isTransmissionTimer(message)){
        handleTransmissionTimer(message);
    }
    else if (isReceptionTimer(message)){
        handleReceptionTimer(message);
    }
    else
        throw cRuntimeError("Unknown self message");
}

bool Sat_Dsl_Phy_CustomRadio::isTransmissionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "transmissionTimer");
}

void Sat_Dsl_Phy_CustomRadio::handleTransmissionTimer(cMessage *message)
{
    if (message->getKind() == IRadioSignal::SIGNAL_PART_WHOLE)
        endTransmission(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_PREAMBLE)
        continueTransmission(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_HEADER)
        continueTransmission(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_DATA)
        endTransmission(message);
    else
        throw cRuntimeError("Unknown self message");
}

void Sat_Dsl_Phy_CustomRadio::handleUpperPacket(Packet *packet)
{
    emit(packetReceivedFromUpperSignal, packet);
    EV << packet->getDetailStringRepresentation(evFlags) << endl;

    // For custom radio, no LoRa preamble needed
    auto signalPowerReq = packet->addTagIfAbsent<SignalPowerReq>();
    signalPowerReq->setPower(mW(501.2)); // Default power

    cMessage *timer = new cMessage("transmissionTimer");
    timer->setKind(IRadioSignal::SIGNAL_PART_WHOLE);
    
    const auto part = separateTransmissionParts ? IRadioSignal::SIGNAL_PART_PREAMBLE : IRadioSignal::SIGNAL_PART_WHOLE;
    startTransmission(packet, part);
}

void Sat_Dsl_Phy_CustomRadio::startTransmission(Packet *macFrame, IRadioSignal::SignalPart part)
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

    cMessage *timer = new cMessage("transmissionTimer");
    timer->setKind(part);
    timer->setContextPointer(const_cast<WirelessSignal *>(signal));

    scheduleAt(transmission->getEndTime(part), timer);
    concurrentTransmissions.push_back(timer);
    
    EV_INFO << "Transmission started: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    
    iAmTransmiting = true;
    updateTransceiverState();
    updateTransceiverPart();
    emit(transmissionStartedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalDepartureStartedSignal, check_and_cast<const cObject *>(transmission));
}

void Sat_Dsl_Phy_CustomRadio::continueTransmission(cMessage *timer)
{
    auto radioSignal = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(part + 1);
    auto transmission = radioSignal->getTransmission();
    
    EV_INFO << "Transmission continues: " << (IWirelessSignal *)radioSignal << " " << IRadioSignal::getSignalPartName(part) << " -> " << IRadioSignal::getSignalPartName(nextPart) << " as " << transmission << endl;
    
    timer->setKind(nextPart);
    scheduleAt(transmission->getEndTime(nextPart), timer);
    
    updateTransceiverPart();
}

void Sat_Dsl_Phy_CustomRadio::endTransmission(cMessage *timer)
{
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto signal = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto transmission = signal->getTransmission();
    timer->setContextPointer(nullptr);
    
    EV_INFO << "Transmission ended: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    
    concurrentTransmissions.remove(timer);
    delete timer;
    
    if (concurrentTransmissions.empty()) {
        iAmTransmiting = false;
    }
    
    updateTransceiverState();
    updateTransceiverPart();
    emit(transmissionEndedSignal, check_and_cast<const cObject *>(transmission));
}

void Sat_Dsl_Phy_CustomRadio::handleSignal(WirelessSignal *radioFrame)
{
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
}

bool Sat_Dsl_Phy_CustomRadio::isReceptionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "receptionTimer");
}

void Sat_Dsl_Phy_CustomRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = signal->getArrival();
    auto reception = signal->getReception();
    
    if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
        auto transmission = signal->getTransmission();
        auto decision = medium->getReceptionDecision(this, signal->getListening(), transmission, part);
        const bool isReceptionPossible = decision->isReceptionPossible();
        
        EV << "Sat_Dsl_Phy_CustomRadio::startReception - Checking reception possibility for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << ": " << (isReceptionPossible ? "possible" : "impossible") << endl;
        
        if (isReceptionPossible) {
            EV << "Reception started: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
            Sat_Dsl_Phy_CustomRadioReceptionStarted_counter++;
            emit(Sat_Dsl_Phy_CustomRadioReceptionStarted, satIndex);
        }
        
        concurrentReceptions.push_back(timer);
        emit(receptionStartedSignal, check_and_cast<const cObject *>(reception));
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

void Sat_Dsl_Phy_CustomRadio::continueReception(cMessage *timer)
{
    auto radioSignal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(part + 1);
    auto arrival = radioSignal->getArrival();
    auto reception = radioSignal->getReception();
    
    EV_INFO << "Reception continues: " << (IWirelessSignal *)radioSignal << " " << IRadioSignal::getSignalPartName(part) << " -> " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
    
    timer->setKind(nextPart);
    scheduleAt(arrival->getEndTime(nextPart), timer);
    updateTransceiverPart();
}

void Sat_Dsl_Phy_CustomRadio::endReception(cMessage *timer)
{
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival = signal->getArrival();
    auto reception = signal->getReception();

    if (isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
        auto transmission = signal->getTransmission();
        auto decision = medium->getReceptionDecision(this, signal->getListening(), transmission, part);
        const bool isReceptionSuccessful = decision->isReceptionSuccessful();
        
        EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for "
                << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        auto macFrame = medium->receivePacket(this, signal);
        take(macFrame);

        if (isReceptionSuccessful) {
            Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect_counter++;
            emit(Sat_Dsl_Phy_CustomRadioReceptionFinishedCorrect, satIndex);
            sendUp(macFrame);
        }
        else {
            countReceptionsFinishedIncorrect++;
            delete macFrame;
        }
        
        emit(receptionEndedSignal, check_and_cast<const cObject *>(reception));
    }
    else {
        EV_INFO << "Reception ended: ignoring " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        emit(Sat_Dsl_Phy_CustomRadioReceptionFinishedIgnoring, satIndex);
    }
    
    concurrentReceptions.remove(timer);
    updateTransceiverState();
    updateTransceiverPart();
    delete timer;
    check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
}

void Sat_Dsl_Phy_CustomRadio::abortReception(cMessage *timer)
{
    auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto reception = signal->getReception();
    
    EV_INFO << "Reception aborted: for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    
    concurrentReceptions.remove(timer);
    updateTransceiverState();
    updateTransceiverPart();
}

void Sat_Dsl_Phy_CustomRadio::completeRadioModeSwitch(RadioMode newRadioMode)
{
    // Implementation if needed
}

}

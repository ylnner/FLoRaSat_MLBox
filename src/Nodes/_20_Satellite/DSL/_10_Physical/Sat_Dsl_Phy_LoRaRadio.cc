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

#include "Sat_Dsl_Phy_LoRaRadio.h"

#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

#include "Global/Medium/DSL/LoRaMedium.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"

using namespace mac;
using namespace messages;
using namespace satellite;
using namespace medium;

namespace radio {

Define_Module(Sat_Dsl_Phy_LoRaRadio);

void Sat_Dsl_Phy_LoRaRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    iAmGateway = par("iAmGateway").boolValue();
    satIndex = par("satIndex");
    if (stage == INITSTAGE_LAST) {
        setRadioMode(RADIO_MODE_TRANSCEIVER);
        Sat_Dsl_Phy_LoRaRadioReceptionStarted = registerSignal("Sat_Dsl_Phy_LoRaRadioReceptionStarted");
        Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect = registerSignal("Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect");
        Sat_Dsl_Phy_LoRaRadioReceptionFinishedIgnoring = registerSignal("Sat_Dsl_Phy_LoRaRadioReceptionFinishedIgnoring");
        Sat_Dsl_Phy_LoRaRadioReceptionStarted_counter = 0;
        Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect_counter = 0;

        Sat_Dsl_Phy_TranmissionStatisticsCorrect = registerSignal("Sat_Dsl_Phy_TranmissionStatisticsCorrect");
        Sat_Dsl_Phy_TranmissionStatisticsIgnoring = registerSignal("Sat_Dsl_Phy_TranmissionStatisticsIgnoring");
        iAmTransmiting = false;
    }
}

void Sat_Dsl_Phy_LoRaRadio::finish()
{
    FlatRadioBase::finish();
    recordScalar("DER - Data Extraction Rate", double(Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect_counter)/Sat_Dsl_Phy_LoRaRadioReceptionStarted_counter);
    recordScalar("Number of receptions finished with error", countReceptionsFinishedIncorrect);
}

void Sat_Dsl_Phy_LoRaRadio::handleSelfMessage(cMessage *message)
{
    EV_INFO << "Sat_Dsl_Phy_LoRaRadio::handleSelfMessage" << endl;
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


bool Sat_Dsl_Phy_LoRaRadio::isTransmissionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "transmissionTimer");
}

void Sat_Dsl_Phy_LoRaRadio::handleTransmissionTimer(cMessage *message)
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

void Sat_Dsl_Phy_LoRaRadio::handleUpperPacket(Packet *packet)
{
    const auto &frame = packet->peekAtFront<Base_MacFrame>();
    //if(frame->getPktType()==ACK){
    emit(packetReceivedFromUpperSignal, packet);

    EV_INFO << packet->getDetailStringRepresentation(evFlags) << endl;
    //const auto &frame = packet->peekAtFront<LoRaMacFrame>();

    auto preamble = makeShared<LoRaPhyPreamble>();

    preamble->setBandwidth(frame->getLoRaBW());
    preamble->setCenterFrequency(frame->getLoRaCF());
    preamble->setCodeRendundance(frame->getLoRaCR());
    preamble->setPower(mW(frame->getLoRaTP()));
    preamble->setSpreadFactor(frame->getLoRaSF());
    preamble->setUseHeader(frame->getLoRaUseHeader());
    preamble->setReceiverAddress(frame->getReceiverAddress());
//    const auto & loraHeader =  packet->peekAtFront<LoRaMacFrame>();
//    preamble->setReceiverAddress(loraHeader->getReceiverAddress());
//
    auto signalPowerReq = packet->addTagIfAbsent<SignalPowerReq>();
    signalPowerReq->setPower(mW(frame->getLoRaTP()));
//
    preamble->setChunkLength(b(16));
    packet->insertAtFront(preamble);
    EV_INFO << "I send " << preamble->getPower() << " with SF " << preamble->getSpreadFactor() << endl;

    //if (frame->getPktType()==BEACON){

    if (separateTransmissionParts)
        startTransmission(packet, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startTransmission(packet, IRadioSignal::SIGNAL_PART_WHOLE);
    //}
}

void Sat_Dsl_Phy_LoRaRadio::startTransmission(Packet *macFrame, IRadioSignal::SignalPart part)
{
    if(iAmTransmiting == false)
    {
        iAmTransmiting = true;
        auto radioFrame = createSignal(macFrame);
        auto transmission = radioFrame->getTransmission();

        if(macFrame->findTag<inet::TransmissionTimeTag>()){
            // transmission delay
            EV_INFO << "findTag TransmissionTimeTag"<<endl;
            simtime_t transmissionFinishTime = transmission->getEndTime(); //channel->getTransmissionFinishTime();
            simtime_t transmissionTime = transmissionFinishTime - simTime();
            ASSERT(transmissionTime > 0);

            // INET 4.5: TransmissionTimeTag uses appendBitTotalTimes() instead of appendTotalTimes()
            macFrame->getTagForUpdate<inet::TransmissionTimeTag>()->appendBitTotalTimes(transmissionTime);
            EV_INFO << "transmissionFinishTime: " << transmissionFinishTime<<endl;
            // init for propagation delay
            macFrame->addTagIfAbsent<SendAtTag>()->setSendAt(transmissionFinishTime);
        }

        cMessage *txTimer = new cMessage("transmissionTimer");
        txTimer->setKind(part);
        txTimer->setContextPointer(radioFrame);
        scheduleAt(transmission->getEndTime(part), txTimer);
        emit(transmissionStartedSignal, check_and_cast<const cObject *>(transmission));

        EV_INFO << "Transmission started 111: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
        check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalDepartureStartedSignal, check_and_cast<const cObject *>(transmission));
    }
    else delete macFrame;
}

void Sat_Dsl_Phy_LoRaRadio::continueTransmission(cMessage *timer)
{
    auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame = static_cast<IWirelessSignal *>(timer->getContextPointer());
    auto transmission = radioFrame->getTransmission();
    EV_INFO << "Transmission ended: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << radioFrame->getTransmission() << endl;
    timer->setKind(nextPart);
    scheduleAt(transmission->getEndTime(nextPart), timer);
    EV_INFO << "Transmission started 222: " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << transmission << endl;
}

void Sat_Dsl_Phy_LoRaRadio::endTransmission(cMessage *timer)
{
    iAmTransmiting = false;
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto signal = static_cast<WirelessSignal *>(timer->getContextPointer());
    auto transmission = signal->getTransmission();
    timer->setContextPointer(nullptr);
//    concurrentTransmissions.remove(timer);
    EV_INFO << "Transmission ended: " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    emit(transmissionEndedSignal, check_and_cast<const cObject *>(transmission));
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalDepartureEndedSignal, check_and_cast<const cObject *>(transmission));
    delete(timer);
}

void Sat_Dsl_Phy_LoRaRadio::handleSignal(WirelessSignal *radioFrame)
{
    EV_INFO << "Sat_Dsl_Phy_LoRaRadio::handleSignal" <<endl;
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
}

bool Sat_Dsl_Phy_LoRaRadio::isReceptionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "receptionTimer");
}

void Sat_Dsl_Phy_LoRaRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    EV_INFO << "Sat_Dsl_Phy_LoRaRadio::startReception" <<endl;
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival    = radioFrame->getArrival();
    auto reception  = radioFrame->getReception();
    ////emit(Sat_Dsl_Phy_LoRaRadioReceptionStarted, satIndex);

    if (simTime() >= getSimulation()->getWarmupPeriod())
        Sat_Dsl_Phy_LoRaRadioReceptionStarted_counter++;

    if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime() && iAmTransmiting == false) {
        auto transmission = radioFrame->getTransmission();
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, part);
        EV_INFO << "isReceptionAttempted: " << isReceptionAttempted <<endl;
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception started: " << (isReceptionAttempted ? "attempting" : "not attempting") << " -- " << (WirelessSignal *)radioFrame << " -- " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        if (isReceptionAttempted) {
            if(iAmGateway) {
                concurrentReceptions.push_back(timer);
            }
            receptionTimer = timer;
        }
    }
    else
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception started: ignoring " << (WirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    timer->setKind(part);
    scheduleAt(arrival->getEndTime(part), timer);
    //updateTransceiverState();
    //updateTransceiverPart();
    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject *>(reception));
    if(iAmGateway) EV_INFO << "[MSDebug] start reception, size : " << concurrentReceptions.size() << endl;
}

void Sat_Dsl_Phy_LoRaRadio::continueReception(cMessage *timer)
{
    auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart     = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame   = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival      = radioFrame->getArrival();
    auto reception    = radioFrame->getReception();
    if(iAmGateway) {
        std::list<cMessage *>::iterator it;
        for (it=concurrentReceptions.begin(); it!=concurrentReceptions.end(); it++) {
            if(*it == timer) receptionTimer = timer;
        }
    }
    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime(previousPart) == simTime() && iAmTransmiting == false) {
        auto transmission = radioFrame->getTransmission();
        bool isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, previousPart);
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        if (!isReceptionSuccessful) {
            receptionTimer = nullptr;
            if(iAmGateway) concurrentReceptions.remove(timer);
        }
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, nextPart);
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception started: " << (isReceptionAttempted ? "attempting" : "not attempting") << " " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
        if (!isReceptionAttempted) {
            receptionTimer = nullptr;
            if(iAmGateway) concurrentReceptions.remove(timer);
        }
    }
    else {
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception ended: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception started: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
    }
    timer->setKind(nextPart);
    scheduleAt(arrival->getEndTime(nextPart), timer);
    //updateTransceiverState();
    //updateTransceiverPart();
    radioMode = RADIO_MODE_TRANSCEIVER;
}

void Sat_Dsl_Phy_LoRaRadio::endReception(cMessage *timer)
{
    EV_INFO << "Sat_Dsl_Phy_LoRaRadio::endReception" <<endl;
    auto part       = (IRadioSignal::SignalPart)timer->getKind();
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto arrival    = radioFrame->getArrival();
    auto reception  = radioFrame->getReception();

    std::list<cMessage *>::iterator it;
    if(iAmGateway) {
        for (it=concurrentReceptions.begin(); it!=concurrentReceptions.end(); it++) {
            if(*it == timer) receptionTimer = timer;
        }
    }


    const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(radioFrame->getReception());

    W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
    W sensitivity = check_and_cast<const LoRaReceiver *>(radioFrame->getReceiver()->getReceiver())->getSensitivity(loRaReception);

    double minReceptionPower_dBm = math::mW2dBmW(double(minReceptionPower.get()*1000));
    double sensitivity_dBm = math::mW2dBmW(double(sensitivity.get()*1000));

    EV << "minReceptionPower: " << minReceptionPower <<endl;
    EV << "minReceptionPower: " << minReceptionPower_dBm << " dBm "<<endl;

    EV << "Receiver sensitivity is " << sensitivity << endl;
    EV << "Received sensitivity is " << sensitivity_dBm << " dBm" << endl;


    // 1. Crear el contenedor de datos
    cValueMap *statsMap_error = new cValueMap("TransmissionStats");


    // 2. Insertar los valores
    //statsMap->set("macFrame", macFrame); // El mapa puede guardar objetos
    statsMap_error->set("minPower_dBm", minReceptionPower_dBm);
    statsMap_error->set("sensitivity_dBm", sensitivity_dBm);
    statsMap_error->set("duration", (double)radioFrame->getTransmission()->getDuration().dbl());
    statsMap_error->set("doppler", loRaReception->getDopplerShift().get());


    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime() && iAmTransmiting == false) {
        auto transmission = radioFrame->getTransmission();


// TODO: this would draw twice from the random number generator in isReceptionSuccessful: auto isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, part);
        auto isReceptionSuccessful = medium->getReceptionDecision(this, radioFrame->getListening(), transmission, part)->isReceptionSuccessful();

        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        if(isReceptionSuccessful) {
            auto macFrame = medium->receivePacket(this, radioFrame);
            take(macFrame);
            emit(packetSentToUpperSignal, macFrame);
            //////emit(Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect, satIndex);
            if (simTime() >= getSimulation()->getWarmupPeriod())
                Sat_Dsl_Phy_LoRaRadioReceptionFinishedCorrect_counter++;
            EV_INFO << macFrame->getCompleteStringRepresentation(evFlags) << endl;
            EV_INFO << "Sat_Dsl_Phy_LoRaRadio::endReception" <<endl;
            EV_INFO << ">>> EMITIENDO SENAL AHORA CON EL PAQUETE: " << macFrame->getName() << " <<<" << endl;

            EV_INFO << "getStartTime" << radioFrame->getTransmission()->getStartTime() << endl;
            EV_INFO << "getEndTime" << radioFrame->getTransmission()->getEndTime() << endl;
            EV_INFO << "getDuration" << radioFrame->getTransmission()->getDuration() << endl;

            /*
            const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(radioFrame->getReception());

            W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
            W sensitivity = check_and_cast<const LoRaReceiver *>(radioFrame->getReceiver()->getReceiver())->getSensitivity(loRaReception);
            double minReceptionPower_dBm = math::mW2dBmW(double(minReceptionPower.get()*1000));
            EV << "minReceptionPower: " << minReceptionPower <<endl;
            EV << "minReceptionPower: " << minReceptionPower_dBm << " dBm "<<endl;
            double sensitivity_dBm = math::mW2dBmW(double(sensitivity.get()*1000));
            EV << "Receiver sensitivity is " << sensitivity << endl;
            EV << "Received sensitivity is " << sensitivity_dBm << " dBm" << endl;


            // ACHF
            //cValueMap *map = new cValueMap("LoRa_Receiver");
            //map->set("minPower_dBm", minReceptionPower_dBm);
            //map->set("sensitivity_dBm", sensitivity_dBm);
            */
            // 1. Crear el contenedor de datos
            cValueMap *statsMap = new cValueMap("TransmissionStats");

            // 2. Insertar los valores
            statsMap->set("macFrame", macFrame->dup()); // El mapa puede guardar objetos
            statsMap->set("minPower_dBm", minReceptionPower_dBm);
            statsMap->set("sensitivity_dBm", sensitivity_dBm);
            statsMap->set("duration", (double)radioFrame->getTransmission()->getDuration().dbl());
            //const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(radioFrame->getReception());
            statsMap->set("doppler", loRaReception->getDopplerShift().get());



            // 3. Emitir el mapa completo
            //emit(Sat_Dsl_Phy_TranmissionStatisticsCorrect, macFrame);
            emit(Sat_Dsl_Phy_TranmissionStatisticsCorrect, statsMap);
            sendUp(macFrame);
        }else{
            ////////////////////////////emit(Sat_Dsl_Phy_TranmissionStatisticsIgnoring, medium->receivePacket(this, radioFrame));
            statsMap_error->set("macFrame", medium->receivePacket(this, radioFrame));
            emit(Sat_Dsl_Phy_TranmissionStatisticsIgnoring, statsMap_error);
        }
        receptionTimer = nullptr;
        if(iAmGateway) concurrentReceptions.remove(timer);
    } else {
        ////////////////////////////emit(Sat_Dsl_Phy_TranmissionStatisticsIgnoring, medium->receivePacket(this, radioFrame));
        statsMap_error->set("macFrame", medium->receivePacket(this, radioFrame));
        emit(Sat_Dsl_Phy_TranmissionStatisticsIgnoring, statsMap_error);
        emit(Sat_Dsl_Phy_LoRaRadioReceptionFinishedIgnoring, satIndex);
        countReceptionsFinishedIncorrect++;
        EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception ended: ignoring " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    }
    //updateTransceiverState();
    //updateTransceiverPart();
    radioMode = RADIO_MODE_TRANSCEIVER;
    check_and_cast<LoRaMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
    delete timer;
}

/*void Sat_Dsl_Phy_LoRaRadio::sendUp(Packet *macFrame)
{
    EV_INFO << "Sending up " << macFrame << endl;
    send(macFrame, getParentModule()->gate("upperOut"));
}*/

void Sat_Dsl_Phy_LoRaRadio::abortReception(cMessage *timer)
{
    auto radioFrame = static_cast<WirelessSignal *>(timer->getControlInfo());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto reception = radioFrame->getReception();
    EV_INFO << "Sat_Dsl_Phy_LoRaRadio Reception aborted: for " << (IWirelessSignal *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    if (timer == receptionTimer) {
        if(iAmGateway) concurrentReceptions.remove(timer);
        receptionTimer = nullptr;
    }
    updateTransceiverState();
    updateTransceiverPart();
}

}

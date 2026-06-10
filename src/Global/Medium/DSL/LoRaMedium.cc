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
#include "LoRaMedium.h"

#include <cmath>

#include "../../../Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
//#include "inet/linklayer/contract/IMACFrame.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IInterference.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/Radio.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IErrorModel.h"

#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"
#include "Global/Transceiver/LoRaTransmission.h"

#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"



using namespace physicallayer;
using namespace mobility;
using namespace mac;

namespace {

double computeTerminalSatelliteElevationDeg(double terminalLatDeg, double terminalLonDeg, double terminalAltKm,
                                           double satelliteLatDeg, double satelliteLonDeg, double satelliteAltKm)
{
    cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
    cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

    const double dx = satelliteEcef.getX() - terminalEcef.getX();
    const double dy = satelliteEcef.getY() - terminalEcef.getY();
    const double dz = satelliteEcef.getZ() - terminalEcef.getZ();

    const double latRad = deg2rad(terminalLatDeg);
    const double lonRad = deg2rad(terminalLonDeg);
    const double sinLat = std::sin(latRad);
    const double cosLat = std::cos(latRad);
    const double sinLon = std::sin(lonRad);
    const double cosLon = std::cos(lonRad);

    const double east = -sinLon * dx + cosLon * dy;
    const double north = -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz;
    const double up = cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz;

    const double horizontal = std::sqrt(east * east + north * north);
    if (horizontal == 0.0 && up == 0.0)
        return 90.0;
    return rad2deg(std::atan2(up, horizontal));
}

}

namespace medium {

Define_Module(LoRaMedium);

LoRaMedium::LoRaMedium() : RadioMedium()
{
}

LoRaMedium::~LoRaMedium()
{
}

void LoRaMedium::initialize(int stage)
{
    RadioMedium::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        minimumElevationTermSat = par("minimumElevationTermSat").doubleValue();
        mapX = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapY = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 1));
    }
}

bool LoRaMedium::matchesMacAddressFilter(const IRadio *radio, const Packet *packet) const
{
    const auto &chunk = packet->peekAtFront<Chunk>();
    const auto & loraHeader = dynamicPtrCast<const Base_MacFrame>(chunk);
    if (loraHeader == nullptr)
        return false;
    MacAddress address = MacAddress(loraHeader->getReceiverAddress().getInt());
    if (address.isBroadcast() || address.isMulticast())
        return true;

    cModule *host = getContainingNode(check_and_cast<const cModule *>(radio));
    IInterfaceTable *interfaceTable = check_and_cast<IInterfaceTable *>(host->getSubmodule("interfaceTable"));
    for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
        auto interface = interfaceTable->getInterface(i);
        if (interface && interface->getMacAddress() == address)
            return true;
    }
    return false;
}

bool LoRaMedium::isPotentialReceiver(const IRadio *radio, const ITransmission *transmission) const
{
    // EV_INFO << "LoRaMedium::isPotentialReceiver" <<endl;
    const Radio *receiverRadio = dynamic_cast<const Radio *>(radio);
    if (radioModeFilter && receiverRadio != nullptr && receiverRadio->getRadioMode() != IRadio::RADIO_MODE_RECEIVER && receiverRadio->getRadioMode() != IRadio::RADIO_MODE_TRANSCEIVER)
        return false;
    else if (listeningFilter && radio->getReceiver() != nullptr && !radio->getReceiver()->computeIsReceptionPossible(getListening(radio, transmission), transmission))
        return false;
    // TODO where is the tag?
    else if (macAddressFilter && !matchesMacAddressFilter(radio, transmission->getPacket()))
        return false;
    else if (rangeFilter == RANGE_FILTER_INTERFERENCE_RANGE) {
        const IArrival *arrival = getArrival(radio, transmission);
        return isInInterferenceRange(transmission, arrival->getStartPosition(), arrival->getEndPosition());
    }
    else if (rangeFilter == RANGE_FILTER_COMMUNICATION_RANGE) {
        const IArrival *arrival = getArrival(radio, transmission);
        globalPotentialReceiver = radio;
        // EV_INFO << "Previous isInCommunicationRange" <<endl;
        return isInCommunicationRange(transmission, arrival->getStartPosition(), arrival->getEndPosition());
    }
    else
        return true;
}

bool LoRaMedium::isInCommunicationRange(const ITransmission *transmission, const Coord& startPosition, const Coord& endPosition) const
{
    // EV_INFO << "LoRaMedium::isInCommunicationRange" <<endl;
    m maxCommunicationRange = mediumLimitCache->getMaxCommunicationRange();

    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    if( (globalPotentialReceiver != nullptr) && (dynamic_cast<const Sat_Mob_SatelliteMobility *>(globalPotentialReceiver->getAntenna()->getMobility())) &&
            (dynamic_cast<const Sat_Mob_SatelliteMobility *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))){
        EV_INFO << "Potential receiver is SatMobility" <<endl;
        globalPotentialReceiver = nullptr;
        return false;
    }

    if( (globalPotentialReceiver != nullptr) && (dynamic_cast<const Ter_Mob *>(globalPotentialReceiver->getAntenna()->getMobility())) &&
            (dynamic_cast<const Ter_Mob *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))){
        EV_INFO << "Transmissions between end devices" <<endl;
        globalPotentialReceiver = nullptr;
        return false;
    }

    if( (globalPotentialReceiver != nullptr) && (dynamic_cast<const Sat_Mob_SatelliteMobility *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))
            && (dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility())) ){
        EV_INFO << "Sender is SatMobility and potential receiver is GS" <<endl;
        globalPotentialReceiver = nullptr;
        return false;
    }


    // If sender is GS then return false except when potential receiver is GS, if that's the case check if it is the same GS
    if( (globalPotentialReceiver != nullptr) && (dynamic_cast<const Sta_Mob_StationMobility *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))){
        EV_INFO << "Transmissions source is Gs" <<endl;

        // If potential receiver is GS, check if they are the same
        if(dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility())){
            double lat_1 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility()))->getLatitude(), 2);
            double log_1 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility()))->getLongitude(), 2);

            double lat_2 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))->getLatitude(), 2);
            double log_2 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(loRaTransmission->getTransmitter()->getAntenna()->getMobility()))->getLongitude(), 2);

            globalPotentialReceiver = nullptr;

            if ((lat_1 != lat_2) || (log_1 != log_2)){
                return false;
            }else{
                EV_INFO << "Same groundstations"<<endl;
                EV_INFO << "lat_1: " << lat_1 <<endl;
                EV_INFO << "log_1: " << log_1 <<endl;
                EV_INFO << "lat_2: " << lat_2 <<endl;
                EV_INFO << "log_2: " << log_2 <<endl;
            }

        }else{
            globalPotentialReceiver = nullptr;
            return false;
        }
    }

    const IMobility *transmitterMobility = loRaTransmission->getTransmitter()->getAntenna()->getMobility();
    const IMobility *receiverMobility = globalPotentialReceiver ? globalPotentialReceiver->getAntenna()->getMobility() : nullptr;
    const auto *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility *>(transmitterMobility);
    const auto *receiverSatMobility = receiverMobility ? dynamic_cast<const Sat_Mob_SatelliteMobility *>(receiverMobility) : nullptr;
    const auto *transmitterTerMobility = dynamic_cast<const Ter_Mob *>(transmitterMobility);
    const auto *receiverTerMobility = receiverMobility ? dynamic_cast<const Ter_Mob *>(receiverMobility) : nullptr;

    double terminalLatDeg = 0.0;
    double terminalLonDeg = 0.0;
    double terminalAltKm = 0.0;
    double satelliteLatDeg = 0.0;
    double satelliteLonDeg = 0.0;
    double satelliteAltKm = 0.0;
    bool hasElevationMetric = false;

    if (transmitterTerMobility && receiverSatMobility) {
        terminalLatDeg = transmitterTerMobility->getLatitude();
        terminalLonDeg = transmitterTerMobility->getLongitude();
        terminalAltKm = loRaTransmission->getStartLongLatPosition().m_Alt;
        satelliteLatDeg = receiverSatMobility->getLatitude();
        satelliteLonDeg = receiverSatMobility->getLongitude();
        satelliteAltKm = receiverSatMobility->getAltitude();
        hasElevationMetric = true;
    }
    else if (receiverTerMobility && transmitterSatMobility) {
        terminalLatDeg = receiverTerMobility->getLatitude();
        terminalLonDeg = receiverTerMobility->getLongitude();
        terminalAltKm = 0.0;
        satelliteLatDeg = transmitterSatMobility->getLatitude();
        satelliteLonDeg = transmitterSatMobility->getLongitude();
        satelliteAltKm = transmitterSatMobility->getAltitude();
        hasElevationMetric = true;
    }

    if (hasElevationMetric) {
        double elevationDeg = computeTerminalSatelliteElevationDeg(terminalLatDeg, terminalLonDeg, terminalAltKm,
                                                                   satelliteLatDeg, satelliteLonDeg, satelliteAltKm);
        EV_INFO << "Elevation between terminal and satellite is " << elevationDeg << " deg (min " << minimumElevationTermSat << ")" << endl;
        bool isVisible = elevationDeg >= minimumElevationTermSat;
        globalPotentialReceiver = nullptr;
        return isVisible;
    }

    double transmitterLat = loRaTransmission->getStartLongLatPosition().m_Lat;
    double transmitterLon = loRaTransmission->getStartLongLatPosition().m_Lon;
    double transmitterAlt = loRaTransmission->getStartLongLatPosition().m_Alt;

    double receiverX = startPosition.getX();
    double receiverY = startPosition.getY();
    double receiverZ = startPosition.getZ();

    double receiverLon = (360 * receiverX / mapX) - 180;
    double receiverLat = 90 - (180 * receiverY / mapY);
    double receiverAlt = receiverZ;

    cEcef transmitterEcef(transmitterLat, transmitterLon, transmitterAlt);
    cEcef receiverEcef(receiverLat, receiverLon, receiverAlt);
    double distance = transmitterEcef.getDistance(receiverEcef);

    return std::isnan(maxCommunicationRange.get()) || (distance < maxCommunicationRange.get());
    /////return false;
    //return std::isnan(maxCommunicationRange.get()) || (distance < range);
}

void LoRaMedium::addTransmission(const IRadio *transmitterRadio, const ITransmission *transmission){
    Enter_Method("addTransmission");
    EV_INFO << "LoRaMedium::addTransmission"<<endl;
    EV_INFO << "transmitterRadio: "<< transmitterRadio<<endl;
    transmissionCount++;
    communicationCache->addTransmission(transmission);
    simtime_t maxArrivalEndTime = transmission->getEndTime();
    communicationCache->mapRadios([&] (const IRadio *receiverRadio) {
        if (receiverRadio != nullptr && receiverRadio != transmitterRadio && receiverRadio->getReceiver() != nullptr) {
            const IArrival *arrival = propagation->computeArrival(transmission, receiverRadio->getAntenna()->getMobility());
            const IntervalTree::Interval *interval = new IntervalTree::Interval(arrival->getStartTime(), arrival->getEndTime(), (void *)transmission);
            const IListening *listening = receiverRadio->getReceiver()->createListening(receiverRadio, arrival->getStartTime(), arrival->getEndTime(), arrival->getStartPosition(), arrival->getEndPosition());
            const simtime_t arrivalEndTime = arrival->getEndTime();

            if (arrivalEndTime > maxArrivalEndTime)
                maxArrivalEndTime = arrivalEndTime;
            communicationCache->setCachedArrival(receiverRadio, transmission, arrival);
            communicationCache->setCachedInterval(receiverRadio, transmission, interval);
            communicationCache->setCachedListening(receiverRadio, transmission, listening);

        }
    });
    communicationCache->setCachedInterferenceEndTime(transmission, maxArrivalEndTime + mediumLimitCache->getMaxTransmissionDuration());
    if (!removeNonInterferingTransmissionsTimer->isScheduled())
        scheduleAt(communicationCache->getCachedInterferenceEndTime(transmission), removeNonInterferingTransmissionsTimer);
    emit(signalAddedSignal, check_and_cast<const cObject *>(transmission));

}

IWirelessSignal *LoRaMedium::createTransmitterSignal(const IRadio *radio, Packet *packet)
{
    EV_INFO << "LoraMedium::createTransmitterSignal" <<endl;
    if (packet != nullptr)
        take(packet);


    auto sequence   = dynamicPtrCast<const SequenceChunk>(packet->peekData());
    int index = 0;
    for (const auto& chunk : sequence->getChunks()) {
        EV << "Chunk #" << index << ": " << chunk->getChunkType()
                << " - length: " << chunk->getChunkLength() << endl;
        chunk->printToStream(EV, cLog::logLevel, 0);
        EV << "\n -"<<endl;
        index = index +1;
    }
    EV << "-------"<<endl;

    auto transmission = radio->getTransmitter()->createTransmission(radio, packet, simTime());
    auto signal = new WirelessSignal(transmission);
    auto duration = transmission->getDuration();

    // transmission delay
    //simtime_t transmissionFinishTime = channel->getTransmissionFinishTime();
    //simtime_t transmissionTime = transmissionFinishTime - simTime();
    //ASSERT(transmissionTime > 0);
    // INET 4.5: TransmissionTimeTag uses appendBitTotalTimes() instead of appendTotalTimes()
    packet->getTagForUpdate<inet::TransmissionTimeTag>()->appendBitTotalTimes(duration);
    // init for propagation delay
    packet->addTagIfAbsent<satellite::SendAtTag>()->setSendAt(simTime() + duration);

    if (duration > mediumLimitCache->getMaxTransmissionDuration())
        throw cRuntimeError("Maximum transmission duration is exceeded");
    signal->setDuration(duration);
    if (packet != nullptr) {
        signal->setName(packet->getName());
        signal->encapsulate(packet);
    }
    return signal;
}
/*
void LoRaMedium::addRadio(const IRadio *radio)
{
    Enter_Method("addRadio");
    communicationCache->addRadio(radio);
    if (neighborCache)
        neighborCache->addRadio(radio);
    mediumLimitCache->addRadio(radio);
    communicationCache->mapTransmissions([&] (const ITransmission *transmission) {
        // Transmiter is satmobility
        if (const SatMobility *senderSatMobility = dynamic_cast<const SatMobility *>(transmission->getTransmitter()->getAntenna()->getMobility())){
            EV_INFO << "Transmiter is satmobility" <<endl;
            // Potential receiver is satmobility
            if(const SatMobility *senderOtherSatMobility = dynamic_cast<const SatMobility *>(radio->getAntenna()->getMobility())){
                EV_INFO << "Potential receiver is satmobility" <<endl;
            }else{
                const IArrival *arrival = propagation->computeArrival(transmission, radio->getAntenna()->getMobility());
                const IListening *listening = radio->getReceiver()->createListening(radio, arrival->getStartTime(), arrival->getEndTime(), arrival->getStartPosition(), arrival->getEndPosition());
                communicationCache->setCachedArrival(radio, transmission, arrival);
                communicationCache->setCachedListening(radio, transmission, listening);
            }
        }else{
            const IArrival *arrival = propagation->computeArrival(transmission, radio->getAntenna()->getMobility());
            const IListening *listening = radio->getReceiver()->createListening(radio, arrival->getStartTime(), arrival->getEndTime(), arrival->getStartPosition(), arrival->getEndPosition());
            communicationCache->setCachedArrival(radio, transmission, arrival);
            communicationCache->setCachedListening(radio, transmission, listening);
        }
    });
    cModule *radioModule = const_cast<cModule *>(check_and_cast<const cModule *>(radio));
    if (radioModeFilter)
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
    if (listeningFilter)
        radioModule->subscribe(IRadio::listeningChangedSignal, this);
    if (macAddressFilter)
        getContainingNode(radioModule)->subscribe(interfaceConfigChangedSignal, this);
    emit(radioAddedSignal, radioModule);
}
*/
const IReceptionResult *LoRaMedium::getReceptionResult(const IRadio *radio, const IListening *listening, const ITransmission *transmission) const
{
    cacheResultGetCount++;
    const IReceptionResult *result = communicationCache->getCachedReceptionResult(radio, transmission);
    if (result)
        cacheResultHitCount++;
    else {

        result   = computeReceptionResult(radio, listening, transmission);
        auto pkt = const_cast<Packet *>(result->getPacket());

        if (!pkt->findTag<SnirInd>()) {
            const ISnir *snir = getSNIR(radio, transmission);
            auto snirInd = pkt->addTagIfAbsent<SnirInd>();
            snirInd->setMinimumSnir(snir->getMin());
            snirInd->setMaximumSnir(snir->getMax());
        }
        if (!pkt->findTag<ErrorRateInd>()) {
            auto errorModel = dynamic_cast<IErrorModel *>(getSubmodule("errorModel"));
            const ISnir *snir = getSNIR(radio, transmission);
            auto errorRateInd = pkt->addTagIfAbsent<ErrorRateInd>(); // TODO: should be done  setPacketErrorRate(packetModel->getPER());
            errorRateInd->setPacketErrorRate(errorModel ? errorModel->computePacketErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
            errorRateInd->setBitErrorRate(errorModel ? errorModel->computeBitErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
            errorRateInd->setSymbolErrorRate(errorModel ? errorModel->computeSymbolErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
        }

        communicationCache->setCachedReceptionResult(radio, transmission, result);
        EV_DEBUG << "Receiving " << transmission << " from medium by " << radio << " arrives as " << result->getReception() << " and results in " << result << endl;
    }
    return result;
}

}

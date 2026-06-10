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

#include "CustomMedium.h"
#include <cmath>

#include "../../../Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/ProtocolTag_m.h"
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
#include "Global/Transceiver/CustomTransmission.h"

#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"
#include "Global/Messages/_30_Network/SendAtTag_m.h"

using namespace physicallayer;
using namespace mobility;
using namespace mac;

namespace
{

    std::pair<double, double> computeTerminalSatelliteElevationAzimuthDeg(double terminalLatDeg, double terminalLonDeg, double terminalAltKm,
                                                                          double satelliteLatDeg, double satelliteLonDeg, double satelliteAltKm)
    {
        EV << "Terminal latitude: " << terminalLatDeg << " deg, longitude: " << terminalLonDeg << " deg, altitude: " << terminalAltKm << " km" << endl;
        EV << "Satellite latitude: " << satelliteLatDeg << " deg, longitude: " << satelliteLonDeg << " deg, altitude: " << satelliteAltKm << " km" << endl;

        cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
        cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

        EV << "Terminal ECEF coordinates: x=" << terminalEcef.getX() << " km, y=" << terminalEcef.getY() << " km, z=" << terminalEcef.getZ() << " km" << endl;
        EV << "Satellite ECEF coordinates: x=" << satelliteEcef.getX() << " km, y=" << satelliteEcef.getY() << " km, z=" << satelliteEcef.getZ() << " km" << endl;

        const double dx = (satelliteEcef.getX() - terminalEcef.getX()) / 1000.0; // Convert to km;
        const double dy = (satelliteEcef.getY() - terminalEcef.getY()) / 1000.0; // Convert to km;
        const double dz = (satelliteEcef.getZ() - terminalEcef.getZ()) / 1000.0; // Convert to km;

        EV << "Delta ECEF coordinates: dx=" << dx << " km, dy=" << dy << " km, dz=" << dz << " km" << endl;

        const double latRad = deg2rad(terminalLatDeg);
        const double lonRad = deg2rad(terminalLonDeg);
        const double sinLat = std::sin(latRad);
        const double cosLat = std::cos(latRad);
        const double sinLon = std::sin(lonRad);
        const double cosLon = std::cos(lonRad);

        EV << "Terminal sin/cos lat/lon: sinLat=" << sinLat << ", cosLat=" << cosLat << ", sinLon=" << sinLon << ", cosLon=" << cosLon << endl;

        const double east = -sinLon * dx + cosLon * dy;
        const double north = -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz;
        const double up = cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz;

        EV << "Local ENU coordinates: east=" << east << " km, north=" << north << " km, up=" << up << " km" << endl;

        const double horizontal = std::sqrt(east * east + north * north);

        return {rad2deg(std::atan2(up, horizontal)), rad2deg(std::atan2(east, north))};
    }

}

namespace medium
{

    Define_Module(CustomMedium);

    CustomMedium::CustomMedium() : RadioMedium()
    {
    }

    CustomMedium::~CustomMedium()
    {
    }

    void CustomMedium::initialize(int stage)
    {
        RadioMedium::initialize(stage);
        if (stage == INITSTAGE_LOCAL)
        {
            minimumElevationTermSat = par("minimumElevationTermSat").doubleValue();
            mapX = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 0));
            mapY = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 1));
            ignoreInterference = par("ignoreInterference").boolValue();
            expectedAzimuthVector = new cOutVector("Expected Receiver Azimuth (deg)");
            expectedElevationVector = new cOutVector("Expected Receiver Elevation (deg)");
        }
    }

    void CustomMedium::finish()
    {
        delete expectedElevationVector;
        delete expectedAzimuthVector;

        for (auto &pair : expectedSatPositionVectors)
        {
            delete pair.second;
        }
        expectedSatPositionVectors.clear();
    }

    bool CustomMedium::matchesMacAddressFilter(const IRadio *radio, const Packet *packet) const
    {
        const auto &chunk = packet->peekAtFront<Chunk>();
        const auto &customHeader = dynamicPtrCast<const Base_MacFrame>(chunk);
        if (customHeader == nullptr)
            return false;
        MacAddress address = MacAddress(customHeader->getReceiverAddress().getInt());
        if (address.isBroadcast() || address.isMulticast())
            return true;

        cModule *host = getContainingNode(check_and_cast<const cModule *>(radio));
        IInterfaceTable *interfaceTable = check_and_cast<IInterfaceTable *>(host->getSubmodule("interfaceTable"));
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++)
        {
            auto interface = interfaceTable->getInterface(i);
            if (interface && interface->getMacAddress() == address)
                return true;
        }
        return false;
    }

    bool CustomMedium::isPotentialReceiver(const IRadio *radio, const ITransmission *transmission) const
    {
        const Radio *receiverRadio = dynamic_cast<const Radio *>(radio);
        if (radioModeFilter && receiverRadio != nullptr && receiverRadio->getRadioMode() != IRadio::RADIO_MODE_RECEIVER && receiverRadio->getRadioMode() != IRadio::RADIO_MODE_TRANSCEIVER)
            return false;
        else if (listeningFilter && radio->getReceiver() != nullptr && !radio->getReceiver()->computeIsReceptionPossible(getListening(radio, transmission), transmission))
            return false;
        else if (macAddressFilter && !matchesMacAddressFilter(radio, transmission->getPacket()))
            return false;
        else if (rangeFilter == RANGE_FILTER_INTERFERENCE_RANGE)
        {
            const IArrival *arrival = getArrival(radio, transmission);
            return isInInterferenceRange(transmission, arrival->getStartPosition(), arrival->getEndPosition());
        }
        else if (rangeFilter == RANGE_FILTER_COMMUNICATION_RANGE)
        {
            const IArrival *arrival = getArrival(radio, transmission);
            globalPotentialReceiver = radio;
            return isInCommunicationRange(transmission, arrival->getStartPosition(), arrival->getEndPosition());
        }
        else
            return true;
    }

    bool CustomMedium::isInCommunicationRange(const ITransmission *transmission, const Coord &startPosition, const Coord &endPosition) const
    {
        EV << "CustomMedium::isInCommunicationRange" << endl;
        m maxCommunicationRange = mediumLimitCache->getMaxCommunicationRange();

        const CustomTransmission *customTransmission = check_and_cast<const CustomTransmission *>(transmission);

        // Satellite-to-satellite transmissions are not allowed
        if ((globalPotentialReceiver != nullptr) && (dynamic_cast<const Sat_Mob_SatelliteMobility *>(globalPotentialReceiver->getAntenna()->getMobility())) &&
            (dynamic_cast<const Sat_Mob_SatelliteMobility *>(customTransmission->getTransmitter()->getAntenna()->getMobility())))
        {
            EV << "Potential receiver is satellite, transmitter is satellite - not allowed" << endl;
            globalPotentialReceiver = nullptr;
            return false;
        }

        // Terminal-to-terminal transmissions are not allowed
        if ((globalPotentialReceiver != nullptr) && (dynamic_cast<const Ter_Mob *>(globalPotentialReceiver->getAntenna()->getMobility())) &&
            (dynamic_cast<const Ter_Mob *>(customTransmission->getTransmitter()->getAntenna()->getMobility())))
        {
            EV << "Terminal-to-terminal transmission - not allowed" << endl;
            globalPotentialReceiver = nullptr;
            return false;
        }

        // Satellite-to-ground-station transmissions are not allowed
        if ((globalPotentialReceiver != nullptr) && (dynamic_cast<const Sat_Mob_SatelliteMobility *>(customTransmission->getTransmitter()->getAntenna()->getMobility())) && (dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility())))
        {
            EV << "Satellite-to-ground-station transmission - not allowed" << endl;
            globalPotentialReceiver = nullptr;
            return false;
        }

        // Ground station can only transmit to itself (loopback for testing)
        if ((globalPotentialReceiver != nullptr) && (dynamic_cast<const Sta_Mob_StationMobility *>(customTransmission->getTransmitter()->getAntenna()->getMobility())))
        {
            EV << "Transmission source is ground station" << endl;

            // If potential receiver is also ground station, check if they are the same
            if (dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility()))
            {
                double lat_1 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility()))->getLatitude(), 2);
                double log_1 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(globalPotentialReceiver->getAntenna()->getMobility()))->getLongitude(), 2);

                double lat_2 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(customTransmission->getTransmitter()->getAntenna()->getMobility()))->getLatitude(), 2);
                double log_2 = core::roundTo((dynamic_cast<const Sta_Mob_StationMobility *>(customTransmission->getTransmitter()->getAntenna()->getMobility()))->getLongitude(), 2);

                globalPotentialReceiver = nullptr;

                if ((lat_1 != lat_2) || (log_1 != log_2))
                {
                    return false;
                }
                else
                {
                    EV << "Same ground stations - allowed" << endl;
                }
            }
            else
            {
                globalPotentialReceiver = nullptr;
                return false;
            }
        }

        const IMobility *transmitterMobility = customTransmission->getTransmitter()->getAntenna()->getMobility();
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

        // Terminal transmitting to satellite
        if (transmitterTerMobility && receiverSatMobility)
        {
            terminalLatDeg = transmitterTerMobility->getLatitude();
            terminalLonDeg = transmitterTerMobility->getLongitude();
            terminalAltKm = customTransmission->getStartLongLatPosition().m_Alt;
            satelliteLatDeg = receiverSatMobility->getLatitude();
            satelliteLonDeg = receiverSatMobility->getLongitude();
            satelliteAltKm = receiverSatMobility->getAltitude();
            hasElevationMetric = true;
        }
        // Satellite transmitting to terminal
        else if (receiverTerMobility && transmitterSatMobility)
        {
            terminalLatDeg = receiverTerMobility->getLatitude();
            terminalLonDeg = receiverTerMobility->getLongitude();
            terminalAltKm = 0.0;
            satelliteLatDeg = transmitterSatMobility->getLatitude();
            satelliteLonDeg = transmitterSatMobility->getLongitude();
            satelliteAltKm = transmitterSatMobility->getAltitude();
            hasElevationMetric = true;
        }

        if (hasElevationMetric)
        {
            auto [elevationDeg, azimuthDeg] = computeTerminalSatelliteElevationAzimuthDeg(terminalLatDeg, terminalLonDeg, terminalAltKm,
                                                                                          satelliteLatDeg, satelliteLonDeg, satelliteAltKm);
            EV << "Elevation between terminal and satellite : " << elevationDeg << " deg (min " << minimumElevationTermSat << ")" << endl;
            bool isVisible = elevationDeg >= minimumElevationTermSat;
            globalPotentialReceiver = nullptr;
            if (isVisible)
            {
                expectedElevationVector->record(elevationDeg);
                expectedAzimuthVector->record(azimuthDeg);
                int roundedLatitude = (int)round(satelliteLatDeg);
                int roundedLongitude = (int)round(satelliteLonDeg);
                double positionEncoded = roundedLatitude * 1000.0 + (roundedLongitude + 180.0);

                // Get node ID from packet and record per-node expected satellite position
                const auto &chunk = transmission->getPacket()->peekAtFront<Chunk>();
                const auto &kiWanFrame = dynamicPtrCast<const KiWanFrame>(chunk);
                if (kiWanFrame != nullptr)
                {
                    int nodeId = kiWanFrame->getSrcId();
                    if (expectedSatPositionVectors.find(nodeId) == expectedSatPositionVectors.end())
                    {
                        std::string vectorName = "Expected Satellite Position for Node " + std::to_string(nodeId);
                        expectedSatPositionVectors[nodeId] = new cOutVector(vectorName.c_str());
                    }
                    expectedSatPositionVectors[nodeId]->record(positionEncoded);
                }
            }
            return isVisible;
        }

        // Fallback for other cases
        double transmitterLat = customTransmission->getStartLongLatPosition().m_Lat;
        double transmitterLon = customTransmission->getStartLongLatPosition().m_Lon;
        double transmitterAlt = customTransmission->getStartLongLatPosition().m_Alt;

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
    }

    void CustomMedium::addTransmission(const IRadio *transmitterRadio, const ITransmission *transmission)
    {
        Enter_Method("addTransmission");
        EV << "CustomMedium::addTransmission" << endl;
        transmissionCount++;
        communicationCache->addTransmission(transmission);
        simtime_t maxArrivalEndTime = transmission->getEndTime();

        communicationCache->mapRadios([&](const IRadio *receiverRadio)
                                      {
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
        } });

        communicationCache->setCachedInterferenceEndTime(transmission, maxArrivalEndTime + mediumLimitCache->getMaxTransmissionDuration());
        if (!removeNonInterferingTransmissionsTimer->isScheduled())
            scheduleAt(communicationCache->getCachedInterferenceEndTime(transmission), removeNonInterferingTransmissionsTimer);
        emit(signalAddedSignal, check_and_cast<const cObject *>(transmission));
    }

    IWirelessSignal *CustomMedium::createTransmitterSignal(const IRadio *radio, Packet *packet)
    {
        EV << "CustomMedium::createTransmitterSignal" << endl;
        if (packet != nullptr)
            take(packet);
        auto transmission = radio->getTransmitter()->createTransmission(radio, packet, simTime());
        auto signal = new WirelessSignal(transmission);
        auto duration = transmission->getDuration();

        // INET 4.3
        // packet->getTagForUpdate<inet::TransmissionTimeTag>()->appendTotalTimes(duration);
        // INET 4.5
        packet->getTagForUpdate<inet::TransmissionTimeTag>()->appendBitTotalTimes(duration);
        packet->addTagIfAbsent<satellite::SendAtTag>()->setSendAt(simTime() + duration);

        EV_INFO << "Creating transmitter signal with duration " << duration << endl;
        EV_INFO << "Maximum allowed transmission duration is " << mediumLimitCache->getMaxTransmissionDuration() << endl;

        if (duration > mediumLimitCache->getMaxTransmissionDuration())
            throw cRuntimeError("Maximum transmission duration is exceeded");
        signal->setDuration(duration);
        if (packet != nullptr)
        {
            signal->setName(packet->getName());
            signal->encapsulate(packet);
        }
        return signal;
    }

    // const std::vector<const IReception *> *CustomMedium::computeInterferingReceptions(const IListening *listening) const
    // {
    //     const IRadio *radio = listening->getReceiver();
    //     std::vector<const ITransmission *> *interferingTransmissions = communicationCache->computeInterferingTransmissions(radio, listening->getStartTime(), listening->getEndTime());
    //     std::vector<const IReception *> *interferingReceptions = new std::vector<const IReception *>();
    //     for (const auto interferingTransmission : *interferingTransmissions)
    //         if (isInterferingTransmission(interferingTransmission, listening))
    //             if (!ignoreInterference) {
    //                 interferingReceptions->push_back(getReception(radio, interferingTransmission));
    //             } else {
    //                 interferingReceptions->push_back(0);
    //             }
    //     delete interferingTransmissions;
    //     return interferingReceptions;
    // }

    const std::vector<const IReception *> *CustomMedium::computeInterferingReceptions(const IReception *reception) const
    {
        EV << "CustomMedium::computeInterferingReceptions" << endl;
        const IRadio *radio = reception->getReceiver();
        const ITransmission *transmission = reception->getTransmission();
        std::vector<const ITransmission *> *interferingTransmissions = communicationCache->computeInterferingTransmissions(radio, reception->getStartTime(), reception->getEndTime());
        std::vector<const IReception *> *interferingReceptions = new std::vector<const IReception *>();
        for (const auto interferingTransmission : *interferingTransmissions)
        {
            if (transmission != interferingTransmission && isInterferingTransmission(interferingTransmission, reception))
            {
                if (!ignoreInterference)
                {
                    interferingReceptions->push_back(getReception(radio, interferingTransmission));
                }
                else
                {
                    interferingReceptions->push_back(getReception(radio, transmission));
                }
            }
        }
        delete interferingTransmissions;
        return interferingReceptions;
    }

    const IReceptionResult *CustomMedium::getReceptionResult(const IRadio *radio, const IListening *listening, const ITransmission *transmission) const
    {
        cacheResultGetCount++;
        const IReceptionResult *result = communicationCache->getCachedReceptionResult(radio, transmission);
        if (result)
            cacheResultHitCount++;
        else
        {
            result = computeReceptionResult(radio, listening, transmission);
            auto pkt = const_cast<Packet *>(result->getPacket());

            if (!pkt->findTag<SnirInd>())
            {
                const ISnir *snir = getSNIR(radio, transmission);
                auto snirInd = pkt->addTagIfAbsent<SnirInd>();
                snirInd->setMinimumSnir(snir->getMin());
                snirInd->setMaximumSnir(snir->getMax());
            }
            if (!pkt->findTag<ErrorRateInd>())
            {
                auto errorModel = dynamic_cast<IErrorModel *>(getSubmodule("errorModel"));
                const ISnir *snir = getSNIR(radio, transmission);
                auto errorRateInd = pkt->addTagIfAbsent<ErrorRateInd>();
                errorRateInd->setPacketErrorRate(errorModel ? errorModel->computePacketErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
                errorRateInd->setBitErrorRate(errorModel ? errorModel->computeBitErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
                errorRateInd->setSymbolErrorRate(errorModel ? errorModel->computeSymbolErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
            }

            communicationCache->setCachedReceptionResult(radio, transmission, result);
            EV_DEBUG << "Receiving " << transmission << " from medium by " << radio << " arrives as " << result->getReception() << " and results in " << result << endl;
        }
        return result;
    }

} // namespace medium

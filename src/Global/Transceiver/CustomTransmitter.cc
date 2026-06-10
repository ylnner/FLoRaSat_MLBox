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

#include "CustomTransmitter.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarTransmission.h"
#include "Global/Channel/LoRaModulation.h"
#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"
#include <algorithm>

#include "../../Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.h"

using namespace messages;
using namespace radio;

namespace transceiver {

Define_Module(CustomTransmitter);

CustomTransmitter::CustomTransmitter() :
    FlatTransmitterBase()
{
}

void CustomTransmitter::initialize(int stage)
{
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        preambleDuration = 0.001;  // 1ms preamble
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));  // Default 401.63 MHz
        bandwidth = Hz(par("bandwidth"));
        transmissionCreatedSignal = registerSignal("transmissionCreated");
        iAmGateway = false;
        if(strcmp(getParentModule()->getClassName(), "radio::Sat_Dsl_Phy_LoRaRadio") == 0)
            iAmGateway = true;
    }
}

std::ostream& CustomTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "CustomTransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

const ITransmission *CustomTransmitter::createTransmission(const IRadio *transmitter, const Packet *macFrame, const simtime_t startTime) const
{
    const_cast<CustomTransmitter* >(this)->emit(transmissionCreatedSignal, true);
    
    EV << macFrame->getDetailStringRepresentation(evFlags) << endl;

    // Calculate transmission duration based on packet size and bitrate
    int payloadBytes = B(macFrame->getDataLength()).get();
    simtime_t duration = payloadBytes * 8 / bitrate.get();  // time = bits / bitrate
    
    // For now, use a fixed short duration (can be made configurable)
    duration = 0.982;  // 0.982s
    
    const simtime_t endTime = startTime + duration;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    W transmissionPower = computeTransmissionPower(macFrame);

    EV << "CustomTransmitter::createTransmission - Transmission Power: " << transmissionPower << endl;

    auto longLatStartPosition = cCoordGeo();
    auto longLatEndPosition = cCoordGeo();

    // Get geographic position based on mobility type
    if (Sat_Mob_SatelliteMobility *sgp4Mobility = dynamic_cast<Sat_Mob_SatelliteMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
        longLatEndPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
    }
    else if (Ter_Mob *diskMobility = dynamic_cast<Ter_Mob *>(mobility))
    {
        longLatStartPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
    }
    else if (Sta_Mob_StationMobility *lutMobility = dynamic_cast<Sta_Mob_StationMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
    }
    else
    {
        EV << "Unknown mobility type, using default position" << endl;
        longLatStartPosition = cCoordGeo(0, 0, 0);
        longLatEndPosition = cCoordGeo(0, 0, 0);
    }

    // Calculate timing durations
    simtime_t Tpreamble = preambleDuration;
    simtime_t Theader = duration * 0.1;  // 10% for header
    simtime_t Tpayload = duration * 0.9;  // 90% for payload

    return new CustomTransmission(
        transmitter,
        macFrame,
        startTime,
        endTime,
        Tpreamble,
        Theader,
        Tpayload,
        startPosition,
        endPosition,
        startOrientation,
        endOrientation,
        transmissionPower,
        centerFrequency,
        bandwidth,
        longLatStartPosition,
        longLatEndPosition);
}

} // namespace transceiver

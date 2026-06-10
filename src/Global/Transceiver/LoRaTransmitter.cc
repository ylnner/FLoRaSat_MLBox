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

#include "LoRaTransmitter.h"
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

Define_Module(LoRaTransmitter);

LoRaTransmitter::LoRaTransmitter() :
    FlatTransmitterBase()
{
}

void LoRaTransmitter::initialize(int stage)
{
    TransmitterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        preambleDuration = 0.001; //par("preambleDuration");
        headerLength = b(par("headerLength"));
        bitrate = bps(par("bitrate"));
        power = W(par("power"));
        centerFrequency = Hz(par("centerFrequency"));
        bandwidth = Hz(par("bandwidth"));
        LoRaTransmissionCreated = registerSignal("LoRaTransmissionCreated");
        iAmGateway = false;
        if(strcmp(getParentModule()->getClassName(), "radio::Sat_Dsl_Phy_LoRaRadio") == 0)
            iAmGateway = true;
    }
}

std::ostream& LoRaTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaTransmitter";
    return FlatTransmitterBase::printToStream(stream, level, evFlags);
}

const ITransmission *LoRaTransmitter::createTransmission(const IRadio *transmitter, const Packet *macFrame, const simtime_t startTime) const
{
    // TransmissionBase *controlInfo = dynamic_cast<TransmissionBase *>(macFrame->getControlInfo());
    // W transmissionPower = controlInfo && !std::isnan(controlInfo->getPower().get()) ? controlInfo->getPower() : power;
    const_cast<LoRaTransmitter* >(this)->emit(LoRaTransmissionCreated, true);
    // const LoRaMacFrame *frame = check_and_cast<const LoRaMacFrame *>(macFrame);
    EV_INFO << macFrame->getDetailStringRepresentation(evFlags) << endl;
    const auto &frame = macFrame->peekAtFront<LoRaPhyPreamble>();

    int nPreamble = 8;
    int payloadBytes = B(macFrame->getDataLength()).get() - B(frame->getChunkLength()).get();

    int payloadSymbNb = 8;
    payloadSymbNb += std::ceil((8*payloadBytes - 4*frame->getSpreadFactor() + 28 + 16 - 20*0) / (4*(frame->getSpreadFactor()-2*0))) * (frame->getCodeRendundance() + 4);
    if(payloadSymbNb < 8) payloadSymbNb = 8;

    simtime_t Tsym = pow(2, frame->getSpreadFactor()) / frame->getBandwidth().get();
    simtime_t Tpreamble = (nPreamble + 4.25) * Tsym;
    simtime_t Theader = 0.5 * (8+payloadSymbNb) * Tsym;
    simtime_t Tpayload = 0.5 * (8+payloadSymbNb) * Tsym;

    /*
     * TO DO: We can set duration = 0, to do the downlink process fast without delay.
     * */
    const simtime_t duration = Tpreamble + Theader + Tpayload;
    ////const simtime_t duration = 0.005;
    const simtime_t endTime = startTime + duration;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    W transmissionPower = computeTransmissionPower(macFrame);

    auto longLatStartPosition = cCoordGeo();
    auto longLatEndPosition = cCoordGeo();

    // transmitter is a satellite
    if (Sat_Mob_SatelliteMobility *sgp4Mobility = dynamic_cast<Sat_Mob_SatelliteMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
        longLatEndPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
    }
    // transmitter is a node with random uniform disk mobility initialization
    else if (Ter_Mob *diskMobility = dynamic_cast<Ter_Mob *>(mobility))
    {
        longLatStartPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(diskMobility->getLatitude(), diskMobility->getLongitude(), 0);
    }
    // transmitter is a node or a ground station with a defined position
    else if (Sta_Mob_StationMobility *lutMobility = dynamic_cast<Sta_Mob_StationMobility *>(mobility))
    {
        longLatStartPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
        longLatEndPosition = cCoordGeo(lutMobility->getLatitude(), lutMobility->getLongitude(), 0);
    }else if(Sat_Mob_SatelliteMobility *sgp4Mobility = dynamic_cast<Sat_Mob_SatelliteMobility *>(mobility)){
        // ACHF - SatMobility for routing satellites
        EV_INFO << "ACHF - SatMobility for routing satellites"<<endl;
        longLatStartPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
        longLatEndPosition = cCoordGeo(sgp4Mobility->getLatitude(), sgp4Mobility->getLongitude(), sgp4Mobility->getAltitude());
    }
    // other, should never reach this point
    else
    {
        EV_INFO << "ACHF - other, should never reach this point"<<endl;
        longLatStartPosition = cCoordGeo(0, 0, 0);
        longLatEndPosition = cCoordGeo(0, 0, 0);
    }


    if(!iAmGateway)
    {
        Ter_Phy_LoRaRadio *radio = check_and_cast<Ter_Phy_LoRaRadio *>(getParentModule());
        radio->setCurrentTxPower(transmissionPower.get());
    }

    return new LoRaTransmission(transmitter,
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
            frame->getCenterFrequency(),
            frame->getSpreadFactor(),
            frame->getBandwidth(),
            frame->getCodeRendundance(),
            longLatStartPosition,
            longLatEndPosition);

}

}

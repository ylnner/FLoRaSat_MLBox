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

#include "LoRaAnalogModel.h"

#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarAnalogModel.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarReception.h"
#include "Global/Transceiver/LoRaReception.h"
#include "Global/Transceiver/LoRaTransmission.h"
#include "Global/Transceiver/LoRaReceiver.h"
#include "Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.h"

#include "inet/common/math/PrimitiveFunctions.h" // Interpolated1DFunction
#include "inet/common/math/Interpolators.h"      // LeftInterpolator

using namespace mobility;
using namespace transceiver;

namespace channel
{

    Define_Module(LoRaAnalogModel);
    // ACHF
    void LoRaAnalogModel::initialize(int stage){
        ScalarAnalogModelBase::initialize(stage);

        if (stage == INITSTAGE_LOCAL) {
            enableDoppler = par("enableDoppler").boolValue();
        }
    }

    std::ostream &LoRaAnalogModel::printToStream(std::ostream &stream, int level, int evFlags) const
    {
        return stream << "LoRaAnalogModel";
    }

    const W LoRaAnalogModel::getBackgroundNoisePower(const LoRaBandListening *listening) const
    {
        // const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
        // Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
        W noisePower = W(math::dBmW2mW(-126.5) / 1000);
        if (listening->getLoRaSF() == 6)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-121) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-118) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-111) / 1000);
        }

        if (listening->getLoRaSF() == 7)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-124) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-122) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-116) / 1000);
        }

        if (listening->getLoRaSF() == 8)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-127) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-125) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-119) / 1000);
        }
        if (listening->getLoRaSF() == 9)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-130) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-128) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-122) / 1000);
        }
        if (listening->getLoRaSF() == 10)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-133) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-130) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-125) / 1000);
        }
        if (listening->getLoRaSF() == 11)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-135) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-132) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-128) / 1000);
        }
        if (listening->getLoRaSF() == 12)
        {
            if (listening->getLoRaBW() == Hz(125000))
                noisePower = W(math::dBmW2mW(-137) / 1000);
            if (listening->getLoRaBW() == Hz(250000))
                noisePower = W(math::dBmW2mW(-135) / 1000);
            if (listening->getLoRaBW() == Hz(500000))
                noisePower = W(math::dBmW2mW(-129) / 1000);
        }
        return noisePower;
    }

    W LoRaAnalogModel::computeReceptionPower(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
    {
        const IRadioMedium *radioMedium = receiverRadio->getMedium();
        const IRadio *transmitterRadio = transmission->getTransmitter();

        const IAntenna *receiverAntenna = receiverRadio->getAntenna();
        const IAntenna *transmitterAntenna = transmitterRadio->getAntenna();

        IMobility *receiverMobility = receiverAntenna->getMobility();
        IMobility *transmitterMobility = transmitterAntenna->getMobility();

        double distance = 0; // m

        // if receiver is a node
        if (const Ter_Mob *receiverGroundMobility = dynamic_cast<const Ter_Mob *>(receiverMobility))
        {
            if (const Sat_Mob_SatelliteMobility *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility *>(transmitterMobility))
                distance = receiverGroundMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());

            if (const Ter_Mob *transmitterGroundMobility = dynamic_cast<const Ter_Mob *>(transmitterMobility))
                distance = receiverGroundMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);
        }

        // if receiver is a satellite standalone
        if (const Sat_Mob_SatelliteMobility_Standalone *receiverSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility_Standalone*>(receiverMobility))
        {
            EV << "SatelliteMobility_Standalone" <<endl;
            if (const Sat_Mob_SatelliteMobility_Standalone *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility_Standalone*>(transmitterMobility))
                distance = receiverSatMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());

            if (const Ter_Mob *transmitterGroundMobility = dynamic_cast<const Ter_Mob*>(transmitterMobility))
                distance = receiverSatMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);

            distance = distance*1000; // SatelliteMobility->getDistance() method returns distance in km
        }

        if (const Sat_Mob_SatelliteMobility *receiverSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility*>(receiverMobility))
        {
            if (const Sat_Mob_SatelliteMobility *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility*>(transmitterMobility))
                distance = receiverSatMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());

            if (const Ter_Mob *transmitterGroundMobility = dynamic_cast<const Ter_Mob *>(transmitterMobility))
                distance = receiverSatMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);

            distance = distance * 1000; // SatelliteMobility->getDistance() method returns distance in km
        }

        // const SatMobility *receiverSatMobility = dynamic_cast<const SatMobility *>(mobility)
        //  if receiver is a satellite
        //    if (const SatMobility *receiverSatMobility = dynamic_cast<const SatMobility*>(receiverMobility))
        //    {
        //        if (const SatMobility *transmitterSatMobility = dynamic_cast<const SatMobility*>(transmitterMobility))
        //            distance = receiverSatMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());
        //
        //        if (const UniformGroundMobility *transmitterGroundMobility = dynamic_cast<const UniformGroundMobility*>(transmitterMobility))
        //            distance = receiverSatMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);
        //
        //        distance = distance*1000; // SatelliteMobility->getDistance() method returns distance in km
        //    }

        const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(transmission->getAnalogModel());
        const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
        const Coord receptionStartPosition = arrival->getStartPosition();
        const Coord receptionEndPosition = arrival->getEndPosition();

        mps propagationSpeed = radioMedium->getPropagation()->getPropagationSpeed();
        Hz centerFrequency = Hz(narrowbandSignalAnalogModel->getCenterFrequency());

        //    const Quaternion transmissionDirection = computeTransmissionDirection(transmission, arrival);
        //    const Quaternion transmissionAntennaDirection = transmission->getStartOrientation() - transmissionDirection;
        //    const Quaternion receptionAntennaDirection = transmissionDirection - arrival->getStartOrientation();

        double transmitterAntennaGain = computeAntennaGain(transmission->getTransmitterAntennaGain(), transmission->getStartPosition(), arrival->getStartPosition(), transmission->getStartOrientation());
        double receiverAntennaGain = computeAntennaGain(receiverRadio->getAntenna()->getGain().get(), arrival->getStartPosition(), transmission->getStartPosition(), arrival->getStartOrientation());
        double pathLoss = radioMedium->getPathLoss()->computePathLoss(propagationSpeed, centerFrequency, m(distance));
        double obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(narrowbandSignalAnalogModel->getCenterFrequency(), transmission->getStartPosition(), receptionStartPosition) : 1;
        W transmissionPower = scalarSignalAnalogModel->getPower();
        EV_INFO << "Transmission Power is " << transmissionPower << endl;
        EV_INFO << "Free Space Path Loss is " << pathLoss << endl;
        EV_INFO << "Transmitter Antenna Gain is " << transmitterAntennaGain << endl;
        EV_INFO << "Receiver Antenna Gain is " << receiverAntennaGain << endl;
        return transmissionPower * std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
    }

    // ACHF
    Hz LoRaAnalogModel::computeDopplerFrequency(const IRadio *receiverRadio, const ITransmission *transmission) const
    {
        const LoRaTransmission *loraTransmission = check_and_cast<const LoRaTransmission *>(transmission);
        Hz originalFreq = loraTransmission->getCenterFrequency(); //getActualTransmissionFrequency();

        const IRadio *transmitterRadio = transmission->getTransmitter();
        IMobility *receiverMobility   = receiverRadio->getAntenna()->getMobility();
        IMobility *transmitterMobility = transmitterRadio->getAntenna()->getMobility();

        const auto *transmitterTerMobility = dynamic_cast<const mobility::Ter_Mob *>(transmitterMobility);
        //const auto *receiverSatMobility    = receiverMobility ? dynamic_cast<const mobility::Sat_Mob_SatelliteMobility *>(receiverMobility) : nullptr;
        const auto *receiverSatMobility    = receiverMobility ? dynamic_cast<const mobility::Sat_Mob_SatelliteMobility_Standalone *>(receiverMobility) : nullptr;


        // Fallback: return original frequency if not the expected mobility types
        if (!transmitterTerMobility || !receiverSatMobility) {
            EV_WARN << "computeDopplerFrequency: unexpected mobility types, returning original frequency." << endl;
            return originalFreq;
        }

        double terminalLatDeg = transmitterTerMobility->getLatitude();
        double terminalLonDeg = transmitterTerMobility->getLongitude();
        double terminalAltKm  = transmitterMobility->getCurrentPosition().z / 1000.0; // m -> km
        double satelliteLatDeg = receiverSatMobility->getLatitude();
        double satelliteLonDeg = receiverSatMobility->getLongitude();
        double satelliteAltKm  = receiverSatMobility->getAltitude();

        cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
        cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

        // Use real ECEF velocity from satellite mobility (m/s)
        Coord rxVel;

        if (auto *satMobility = dynamic_cast<mobility::Sat_Mob_SatelliteMobility_Standalone *>(receiverMobility)) {
            //EV_DETAIL << "Satellite real velocity (m/s): " << satMobility->getRealVelocity() << endl;
            EV_DETAIL << "Satellite real velocity (m/s): " << satMobility->getCurrentVelocityEcef() << endl;
            rxVel = satMobility->getCurrentVelocityEcef();
        }

        // Relative position vector in ECEF (km)
        Coord relativePos(
            satelliteEcef.getX() - terminalEcef.getX(),
            satelliteEcef.getY() - terminalEcef.getY(),
            satelliteEcef.getZ() - terminalEcef.getZ()
        );
        double distance = relativePos.length();
        EV_DETAIL << "computeDopplerFrequency: relative position (km): " << relativePos << ", distance: " << distance << " km" << endl;

        // Convert rxVel from m/s to km/s for consistency with relativePos (km)
        Coord rxVelKmPerS = rxVel / 1000.0;

        double relativeVelAlongLOS = (relativePos.x * rxVelKmPerS.x +
                                      relativePos.y * rxVelKmPerS.y +
                                      relativePos.z * rxVelKmPerS.z) / distance;
        // Positive radial velocity => satellite is approaching terminal
        double radialVelocity = -relativeVelAlongLOS; // km/s

        // Doppler: f_rx = f_tx * (c + v_r) / c
        double c = 299792.458; // km/s
        double dopplerShiftedFreqValue = originalFreq.get() * (c + radialVelocity) / c;
        double dopplerShift = dopplerShiftedFreqValue - originalFreq.get();
        EV_INFO << "calculate DopplerShift: " << dopplerShift <<endl;
        EV_INFO << "computeDopplerFrequency: original=" << originalFreq
                << ", radial_vel=" << radialVelocity
                << " km/s, shifted=" << dopplerShiftedFreqValue / 1e6
                << " MHz (shift=" << dopplerShift / 1e6 << " MHz)" << endl;

        return Hz(dopplerShiftedFreqValue);
    }

    const IReception *LoRaAnalogModel::computeReception(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
    {
        const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
        const simtime_t receptionStartTime = arrival->getStartTime();
        const simtime_t receptionEndTime = arrival->getEndTime();
        const Quaternion receptionStartOrientation = arrival->getStartOrientation();
        const Quaternion receptionEndOrientation = arrival->getEndOrientation();
        const Coord receptionStartPosition = arrival->getStartPosition();
        const Coord receptionEndPosition = arrival->getEndPosition();
        W receivedPower = computeReceptionPower(receiverRadio, transmission, arrival);
        Hz LoRaCF = loRaTransmission->getLoRaCF();
        int LoRaSF = loRaTransmission->getLoRaSF();
        Hz LoRaBW = loRaTransmission->getLoRaBW();
        int LoRaCR = loRaTransmission->getLoRaCR();

        LoRaReception *reception = new LoRaReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionStartOrientation, receptionEndOrientation, LoRaCF, LoRaBW, receivedPower, LoRaSF, LoRaCR);

        // ACHF
        Hz receptionFreq = enableDoppler ? computeDopplerFrequency(receiverRadio, transmission) : LoRaCF;
        if (enableDoppler){
            Hz originalFreq = loRaTransmission->getCenterFrequency(); //getActualTransmissionFrequency();
            reception->setDopplerShift(Hz(receptionFreq.get() - originalFreq.get()));
            EV << "reception->getDopplerShift ; "<< reception->getDopplerShift().get()<<endl;
        }

        reception->setDopplerShiftedFrequency(receptionFreq);

        return reception;
    }

    const INoise *LoRaAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
    {
        Hz commonCarrierFrequency;
        Hz commonBandwidth;
        const LoRaBandListening *loRaBandListening = dynamic_cast<const LoRaBandListening *>(listening);
        
        if (loRaBandListening) {
            commonCarrierFrequency = loRaBandListening->getLoRaCF();
            commonBandwidth = loRaBandListening->getLoRaBW();
        } else {
            throw cRuntimeError("LoRaAnalogModel::computeNoise: Unknown listening type");
        }
        
        simtime_t noiseStartTime = SimTime::getMaxTime();
        simtime_t noiseEndTime = 0;
        std::map<simtime_t, W> *powerChanges = new std::map<simtime_t, W>();
        const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
        for (auto reception : *interferingReceptions)
        {
            const ISignalAnalogModel *signalAnalogModel = reception->getAnalogModel();
            const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(signalAnalogModel);
            const LoRaReception *loRaReception = check_and_cast<const LoRaReception *>(signalAnalogModel);
            Hz signalCarrierFrequency = loRaReception->getLoRaCF();
            Hz signalBandwidth = loRaReception->getLoRaBW();
            if ((commonCarrierFrequency == signalCarrierFrequency && commonBandwidth == signalBandwidth))
            {
                const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(signalAnalogModel);
                W power = scalarSignalAnalogModel->getPower();
                simtime_t startTime = reception->getStartTime();
                simtime_t endTime = reception->getEndTime();
                if (startTime < noiseStartTime)
                    noiseStartTime = startTime;
                if (endTime > noiseEndTime)
                    noiseEndTime = endTime;
                std::map<simtime_t, W>::iterator itStartTime = powerChanges->find(startTime);
                if (itStartTime != powerChanges->end())
                    itStartTime->second += power;
                else
                    powerChanges->insert(std::pair<simtime_t, W>(startTime, power));
                std::map<simtime_t, W>::iterator itEndTime = powerChanges->find(endTime);
                if (itEndTime != powerChanges->end())
                    itEndTime->second -= power;
                else
                    powerChanges->insert(std::pair<simtime_t, W>(endTime, -power));
            }
            else if (areOverlappingBands(commonCarrierFrequency, commonBandwidth, narrowbandSignalAnalogModel->getCenterFrequency(), narrowbandSignalAnalogModel->getBandwidth()))
                throw cRuntimeError("Overlapping bands are not supported");
        }

        simtime_t startTime = listening->getStartTime();
        simtime_t endTime = listening->getEndTime();
        std::map<simtime_t, W> *backgroundNoisePowerChanges = new std::map<simtime_t, W>();
        
        // Get background noise power based on listening type
        W noisePower;
        if (loRaBandListening) {
            noisePower = getBackgroundNoisePower(loRaBandListening);
        }
        
        backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(startTime, noisePower));
        backgroundNoisePowerChanges->insert(std::pair<simtime_t, W>(endTime, -noisePower));

        for (const auto &backgroundNoisePowerChange : *backgroundNoisePowerChanges)
        {
            std::map<simtime_t, W>::iterator jt = powerChanges->find(backgroundNoisePowerChange.first);
            if (jt != powerChanges->end())
                jt->second += backgroundNoisePowerChange.second;
            else
                powerChanges->insert(std::pair<simtime_t, W>(backgroundNoisePowerChange.first, backgroundNoisePowerChange.second));
        }

        EV_TRACE << "Noise power begin " << endl;
        W noise = W(0);
        for (std::map<simtime_t, W>::const_iterator it = powerChanges->begin(); it != powerChanges->end(); it++)
        {
            noise += it->second;
            EV_TRACE << "Noise at " << it->first << " = " << noise << endl;
        }
        EV_TRACE << "Noise power end" << endl;

        // Old funtion from INET 4.3:
        // return new ScalarNoise(noiseStartTime, noiseEndTime, commonCarrierFrequency, commonBandwidth, powerChanges);
        // INET 4.5 requires ScalarNoise power to be provided as a math::IFunction.
        // We convert the accumulated delta-based noise power into a left-continuous
        // step function (piecewise constant), which preserves the original semantics
        // of the INET 4.3 noise model.
        std::map<simtime_t, W> cumulative;
        W acc = W(0);
        for (const auto &kv : *powerChanges)
        {
            acc += kv.second;           // kv.second is delta
            cumulative[kv.first] = acc; // store level after applying delta at this time
        }

        // Piecewise-constant function: use left value on each interval
        auto powerFunction = inet::makeShared<inet::math::Interpolated1DFunction<W, simtime_t>>(
            cumulative,
            &inet::math::LeftInterpolator<simtime_t, W>::singleton);

        return new ScalarNoise(noiseStartTime, noiseEndTime, commonCarrierFrequency, commonBandwidth, powerFunction);
    }

    const ISnir *LoRaAnalogModel::computeSNIR(const IReception *reception, const INoise *noise) const
    {
        return new ScalarSnir(reception, noise);
    }

} // namespace inet

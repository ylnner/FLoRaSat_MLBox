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

#include "CustomPHYAnalogModel.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarAnalogModel.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarReception.h"
#include "Global/Transceiver/CustomReception.h"
#include "Global/Transceiver/CustomTransmission.h"
#include "Global/Transceiver/CustomReceiver.h"
#include "Global/Utilities/libnorad/cEcef.h"

using namespace mobility;
using namespace transceiver;

namespace channel {

Define_Module(CustomPHYAnalogModel);

void CustomPHYAnalogModel::initialize(int stage)
{
    ScalarAnalogModelBase::initialize(stage);
    
    if (stage == INITSTAGE_LOCAL) {
        // Load antenna pattern if specified
        std::string antennaPatternFile = par("antennaPatternFile").stdstringValue();
        if (antennaPatternFile.empty()) {
            // Use default isotropic pattern (0 dBi gain in all directions)
            EV_WARN << "CustomPHYAnalogModel: No antennaPatternFile specified, using default isotropic pattern (0 dBi)" << endl;
            transmitterAntennaPattern = AntennaPattern();  // Empty pattern defaults to isotropic
        } else {
            try {
                transmitterAntennaPattern = CSVReader::loadAntennaPattern(antennaPatternFile);
                EV << "  Loaded antenna pattern from: " << antennaPatternFile << endl;
                EV << "  Pattern has " << transmitterAntennaPattern.points.size() << " points" << endl;
            } catch (const std::exception& e) {
                throw cRuntimeError("Failed to load antenna pattern: %s", e.what());
            }
        }
        
        // Initialize vector recorder for received power
        expectedReceivedPowerVector = new cOutVector("Expected Received Power (dBm)");
        txAntennaGainVector = new cOutVector("Transmitter Antenna Gain (dB)");
        rxAntennaGainVector = new cOutVector("Receiver Antenna Gain (dB)");
    }
}

std::ostream& CustomPHYAnalogModel::printToStream(std::ostream& stream, int level, int evFlags) const
{
    return stream << "CustomPHYAnalogModel";
}

W CustomPHYAnalogModel::computeReceptionPower(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const IRadioMedium *radioMedium = receiverRadio->getMedium();
    const IRadio *transmitterRadio = transmission->getTransmitter();

    const IAntenna *receiverAntenna = receiverRadio->getAntenna();
    const IAntenna *transmitterAntenna = transmitterRadio->getAntenna();

    IMobility *receiverMobility = receiverAntenna->getMobility();
    IMobility *transmitterMobility = transmitterAntenna->getMobility();

    double distance = 0; // meters

    // Compute distance between transmitter and receiver based on mobility types
    if (const Ter_Mob *receiverGroundMobility = dynamic_cast<const Ter_Mob*>(receiverMobility))
    {
        if (const Sat_Mob_SatelliteMobility *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility*>(transmitterMobility))
            distance = receiverGroundMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());

        if (const Ter_Mob *transmitterGroundMobility = dynamic_cast<const Ter_Mob*>(transmitterMobility))
            distance = receiverGroundMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);
    }

    if (const Sat_Mob_SatelliteMobility *receiverSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility*>(receiverMobility))
    {
        if (const Sat_Mob_SatelliteMobility *transmitterSatMobility = dynamic_cast<const Sat_Mob_SatelliteMobility*>(transmitterMobility))
            distance = receiverSatMobility->getDistance(transmitterSatMobility->getLatitude(), transmitterSatMobility->getLongitude(), transmitterSatMobility->getAltitude());

        if (const Ter_Mob *transmitterGroundMobility = dynamic_cast<const Ter_Mob*>(transmitterMobility))
            distance = receiverSatMobility->getDistance(transmitterGroundMobility->getLatitude(), transmitterGroundMobility->getLongitude(), 0);

        distance = distance * 1000; // SatelliteMobility->getDistance() returns km
    }

    const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(transmission->getAnalogModel());
    const IScalarSignal *scalarSignalAnalogModel = check_and_cast<const IScalarSignal *>(transmission->getAnalogModel());
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();

    mps propagationSpeed = radioMedium->getPropagation()->getPropagationSpeed();
    Hz centerFrequency = Hz(narrowbandSignalAnalogModel->getCenterFrequency());

    // Compute antenna gains
    // EV << "CustomPHYAnalogModel::computeReceptionPower antenna" << transmission->getTransmitterAntennaGain() << endl;
    // double txAntennaGain = computeAntennaGain(transmission->getTransmitterAntennaGain(), transmission->getStartPosition(), arrival->getStartPosition(), transmission->getStartOrientation());
    
    // For satellite receivers, use omnidirectional azimuth model with elevation-dependent gain
    double rxAntennaGain_dB = 0.0;

    // Compute elevation angle
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

    terminalLatDeg = transmitterTerMobility->getLatitude();
    terminalLonDeg = transmitterTerMobility->getLongitude();
    terminalAltKm = transmission->getStartPosition().z;
    satelliteLatDeg = receiverSatMobility->getLatitude();
    satelliteLonDeg = receiverSatMobility->getLongitude();
    satelliteAltKm = receiverSatMobility->getAltitude();

    EV << "CustomPHYAnalogModel::computeReceptionPower - Terminal (lat, lon, alt): (" << terminalLatDeg << ", " << terminalLonDeg << ", " << terminalAltKm << " km)" << endl;
    EV << "CustomPHYAnalogModel::computeReceptionPower - Satellite (lat, lon, alt): (" << satelliteLatDeg << ", " << satelliteLonDeg << ", " << satelliteAltKm << " km)" << endl;
    
    cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
    cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);
    const double dx = (satelliteEcef.getX() - terminalEcef.getX())/1000.0; // Convert to km;
    const double dy = (satelliteEcef.getY() - terminalEcef.getY())/1000.0; // Convert to km;
    const double dz = (satelliteEcef.getZ() - terminalEcef.getZ())/1000.0; // Convert to km;

    EV << "CustomPHYAnalogModel::computeReceptionPower - Terminal ECEF (x, y, z) in km: (" << terminalEcef.getX() << ", " << terminalEcef.getY() << ", " << terminalEcef.getZ() << ")" << endl;
    EV << "CustomPHYAnalogModel::computeReceptionPower - Satellite ECEF (x, y, z) in km: (" << satelliteEcef.getX() << ", " << satelliteEcef.getY() << ", " << satelliteEcef.getZ() << ")" << endl; 

    EV << "CustomPHYAnalogModel::computeReceptionPower - ECEF delta (dx, dy, dz) in km: (" << dx << ", " << dy << ", " << dz << ")" << endl;

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
    
    double theta_rad = 0.0;
    double theta_deg = 0.0;
    double phi_rad = 0.0;
    double phi_deg = 0.0;

    // Elevation angle
    if (horizontal == 0.0 && up == 0.0) {
        theta_rad = M_PI / 2.0;
        theta_deg = 90.0;
    } else {
        theta_rad = std::atan2(up, horizontal);
        theta_deg = rad2deg(theta_rad);
    }
    
    // Azimuth angle 
    if (east == 0.0 && north == 0.0) {
        phi_rad = 0.0;
        phi_deg = 0.0;
    } else {
        phi_rad = std::atan2(east, north);
        phi_deg = rad2deg(phi_rad);
    }

    EV << "CustomPHYAnalogModel::computeReceptionPower - theta_rad: " << theta_rad << " - theta_deg: " << theta_deg << endl;

    double r_E = 6371.0; // Earth's radius in km
    double r_S = receiverSatMobility->getAltitude() + r_E; // Satellite distance from Earth's center in km
    double psi_rad = asin(r_E/r_S * cos(theta_rad)); // Nadir angle in radians
    double psi_zero_rad = asin(r_E/r_S); // Nadir angle at horizon in radians

    // Perfect function for the antenna reception
    rxAntennaGain_dB = -1.75*cos(3.5*psi_rad/psi_zero_rad)-0.25;

    EV << "Satellite Receiver - Off-nadir angle: " << rad2deg(psi_rad) << "° => Omnidirectional gain: " << rxAntennaGain_dB << " dB" << endl;

    double txAntennaGain = computeAntennaGain(theta_rad, phi_rad);


    // Compute path loss using medium's path loss model (includes FSPL + polarization + fading + system losses)
    double pathLoss = radioMedium->getPathLoss()->computePathLoss(propagationSpeed, centerFrequency, m(distance));
    
    // Compute obstacle loss if available
    double obstacleLoss = radioMedium->getObstacleLoss() ? 
        radioMedium->getObstacleLoss()->computeObstacleLoss(narrowbandSignalAnalogModel->getCenterFrequency(), transmission->getStartPosition(), receptionStartPosition) : 1;
    
    W transmissionPower = scalarSignalAnalogModel->getPower();
    
    // Convert to dB for logging
    double txPowerW = transmissionPower.get();
    double txPower_dBm = 10.0 * log10(txPowerW * 1000.0);
    double pathLoss_dB = (pathLoss > 0.0) ? 10.0 * log10(pathLoss) : -INFINITY;
    double txAntennaGain_dB = (txAntennaGain > 0.0) ? 10.0 * log10(txAntennaGain) : -INFINITY;
    
    EV << "Link budget calculation:" << endl;
    EV << "  Transmission Power: " << transmissionPower << " (" << txPower_dBm << " dBm)" << endl;
    EV << "  Distance: " << distance << " m" << endl;
    EV << "  Path Loss (total): " << pathLoss << " (" << pathLoss_dB << " dB)" << endl;
    EV << "  Transmitter Antenna Gain: " << txAntennaGain << " (" << txAntennaGain_dB << " dB)" << endl;
    EV << "  Receiver Antenna Gain: " << rxAntennaGain_dB << " dB" << endl;

    // Convert receiver antenna gain from dB to linear
    double rxAntennaGain = pow(10.0, rxAntennaGain_dB / 10.0);

    // Account for minor unmodeled effects
    double adjustment_dB = 0.0;
    double adjustment = pow(10.0, adjustment_dB / 10.0);
    
    // Compute received power: Pr = Pt * Gtx * Grx * pathLoss * obstacleLoss * adjustment
    W receivedPower = transmissionPower * std::min(1.0, txAntennaGain * rxAntennaGain * pathLoss * obstacleLoss * adjustment);
    
    // Convert to dBm for logging and recording
    double receivedPower_dBm = 10.0 * log10(receivedPower.get() * 1000.0);
    
    EV << "  Received Power: " << receivedPower << " (" << receivedPower_dBm << " dBm)" << endl;
    
    // Record received power in vector for post-processing
    // expectedReceivedPowerVector->record(receivedPower_dBm);

    // Record components of the link budget for validation
    // txAntennaGainVector->record(txAntennaGain_dB);
    // rxAntennaGainVector->record(rxAntennaGain_dB);
    
    return receivedPower;
}

const IReception *CustomPHYAnalogModel::computeReception(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const CustomTransmission *customTransmission = check_and_cast<const CustomTransmission *>(transmission);
    const simtime_t receptionStartTime = arrival->getStartTime();
    const simtime_t receptionEndTime = arrival->getEndTime();
    const Quaternion receptionStartOrientation = arrival->getStartOrientation();
    const Quaternion receptionEndOrientation = arrival->getEndOrientation();
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
    
    W receivedPower = computeReceptionPower(receiverRadio, transmission, arrival);
    Hz centerFrequency = customTransmission->getCenterFrequency();
    Hz bandwidth = customTransmission->getBandwidth();
    
    return new CustomReception(
        receiverRadio, 
        transmission, 
        receptionStartTime, 
        receptionEndTime, 
        receptionStartPosition, 
        receptionEndPosition, 
        receptionStartOrientation, 
        receptionEndOrientation, 
        centerFrequency, 
        bandwidth, 
        receivedPower);
}

void CustomPHYAnalogModel::finish()
{
    // Clean up vector recorder
    delete expectedReceivedPowerVector;
    delete txAntennaGainVector;
    delete rxAntennaGainVector;
}

double CustomPHYAnalogModel::computeAntennaGain(double theta_rad, double phi_rad) const
{
    bool debug = false;

    // Convert radians to degrees
    double theta = rad2deg(theta_rad);
    double phi = rad2deg(phi_rad);

    if (debug) {
        EV << "computeAntennaGain - Input spherical angles (phi,theta) in degrees: (" << phi << ", " << theta << ")" << endl;
    }

    theta = 90 - theta;

    if (phi < 0.0) {
        phi = phi + 180;
        theta = -theta;
    }

    if (debug) {
        EV << "computeAntennaGain - Mapped spherical angles (phi,theta) in degrees: (" << phi << ", " << theta << ")" << endl;
    }

    // Get gain from pattern (in dBi)
    double gain_dBi = transmitterAntennaPattern.getGain(phi, theta);

    if (debug) {
        EV << "computeAntennaGain - Retrieved gain from pattern: " << gain_dBi << " dBi" << endl;
    }
    
    // Subtract 27 dBm transmission power that was included in the CSV antenna gain values
    gain_dBi -= 27.0;

    if (debug) {
        EV << "computeAntennaGain - Adjusted gain after subtracting transmission power: " << gain_dBi << " dBi" << endl;
    }
    
    // Convert dBi to linear
    return pow(10.0, gain_dBi / 10.0);
}

} // namespace channel

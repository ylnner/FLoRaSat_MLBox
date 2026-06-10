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

#include "CustomAntenna.h"
#include "inet/common/ModuleAccess.h"
#include <cmath>
#include <omnetpp.h>

namespace antenna {

Define_Module(CustomAntenna);

CustomAntenna::AntennaGain::AntennaGain(const AntennaPattern& pattern) :
    pattern(pattern),
    maxGain_dBi(-1000.0),
    minGain_dBi(1000.0)
{
    // Find min and max gains in the pattern
    for (const auto& point : pattern.points) {
        if (point.gain > maxGain_dBi)
            maxGain_dBi = point.gain;
        if (point.gain < minGain_dBi)
            minGain_dBi = point.gain;
    }
}

double CustomAntenna::AntennaGain::getMinGain() const
{
    // Convert dBi to linear
    return pow(10.0, minGain_dBi / 10.0);
}

double CustomAntenna::AntennaGain::getMaxGain() const
{
    // Convert dBi to linear
    return pow(10.0, maxGain_dBi / 10.0);
}

double CustomAntenna::AntennaGain::computeGain(const Quaternion& direction) const
{
    // Convert quaternion to spherical coordinates
    // phi = azimuth angle in xy-plane (-180° to 180°)
    // theta = elevation angle from horizon (-90° to 90°)
    bool debug = false;
    
    double w = direction.s;
    double x = direction.v.x;
    double y = direction.v.y;
    double z = direction.v.z;

    if (debug) { 
    std::cout << "CustomAntenna::AntennaGain::computeGain - Quaternion (w,x,y,z): (" << w << ", " << x << ", " << y << ", " << z << ")" << std::endl;
    }

    // Convert quaternion to direction vector
    // Assuming quaternion rotates the z-axis (0,0,1) to the pointing direction
    double dx = 2.0 * (x * z + w * y);
    double dy = 2.0 * (y * z - w * x);
    double dz = 1.0 - 2.0 * (x * x + y * y);

    if (debug) {
    std::cout << "CustomAntenna::AntennaGain::computeGain - Direction vector (dx,dy,dz): (" << dx << ", " << dy << ", " << dz << ")" << std::endl;
    }
    
    // Calculate spherical coordinates from direction vector
    // phi: azimuth angle in xy-plane (-π to π)
    double phi = atan2(dy, dx);  // Returns angle in range [-π, π]
    
    // theta: elevation angle from horizon (-π/2 to π/2)
    double r_xy = sqrt(dx * dx + dy * dy);
    double theta = atan2(dz, r_xy);  // Returns angle in range [-π/2, π/2]

    if (debug) {
    std::cout << "CustomAntenna::AntennaGain::computeGain - Spherical angles (phi,theta) in radians: (" << phi << ", " << theta << ")" << std::endl;
    }
    
    // Convert from radians to degrees
    phi = phi * 180.0 / M_PI;      // Now in range [-180, 180]
    theta = theta * 180.0 / M_PI;  // Now in range [-90, 90]

    if (debug) {
        std::cout << "CustomAntenna::AntennaGain::computeGain - Spherical angles (phi,theta) in degrees: (" << phi << ", " << theta << ")" << std::endl;
    }

    // Map phi to [0, 180] range and adjust theta if needed
    // The antenna pattern has phi in [0, 180] and theta in [-165, 165]
    if (phi < 0.0) {
        phi = -phi;           // Map to positive: -45° becomes 45°
        theta = theta - 180.0;  // Adjust theta by 180°
    } else if (phi > 180.0) {
        phi = 360.0 - phi;    // Map to [0, 180]: 270° becomes 90°
        theta = theta - 180.0;  // Adjust theta by 180°
    }

    if (debug) {
        std::cout << "CustomAntenna::AntennaGain::computeGain - Mapped spherical angles (phi,theta) in degrees: (" << phi << ", " << theta << ")" << std::endl;
    }

    // Get gain from pattern (in dBi)
    double gain_dBi = pattern.getGain(phi, theta);

    if (debug) {
        std::cout << "CustomAntenna::AntennaGain::computeGain - Retrieved gain from pattern: " << gain_dBi << " dBi" << std::endl;
    }
    
    // Subtract 27 dBm transmission power that was included in the CSV antenna gain values
    gain_dBi -= 27.0;

    if (debug) {
        std::cout << "CustomAntenna::AntennaGain::computeGain - Adjusted gain after subtracting transmission power: " << gain_dBi << " dBi" << std::endl;
    }
        
    // Convert dBi to linear
    return pow(10.0, gain_dBi / 10.0);
}

std::ostream& CustomAntenna::AntennaGain::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "CustomAntennaGain, minGain=" << minGain_dBi << " dBi, maxGain=" << maxGain_dBi << " dBi";
    return stream;
}

void CustomAntenna::initialize(int stage)
{
    AntennaBase::initialize(stage);
    
    if (stage == INITSTAGE_LOCAL) {
        std::string antennaPatternFile = par("antennaPatternFile").stdstringValue();

        if (antennaPatternFile.empty()) {
            // Use default isotropic pattern (0 dBi gain in all directions)
            EV_WARN << "CustomAntenna: No antennaPatternFile specified, using default isotropic pattern (0 dBi)" << endl;
            antennaPattern = AntennaPattern();  // Empty pattern defaults to isotropic
            gain = makeShared<AntennaGain>(antennaPattern);
        } else {
            try {
                antennaPattern = CSVReader::loadAntennaPattern(antennaPatternFile);
                gain = makeShared<AntennaGain>(antennaPattern);
                EV_INFO << "Loaded antenna pattern from: " << antennaPatternFile << endl;
                EV_INFO << "Pattern has " << antennaPattern.points.size() << " points" << endl;
            } catch (const std::exception& e) {
                throw cRuntimeError("Failed to load antenna pattern: %s", e.what());
            }
        }
    }
}

std::ostream& CustomAntenna::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "CustomAntenna";
    if (level <= PRINT_LEVEL_DETAIL)
        stream << ", pattern points: " << antennaPattern.points.size();
    return AntennaBase::printToStream(stream, level);
}

} // namespace antenna

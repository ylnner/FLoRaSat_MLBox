//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef UTILITIES_CSVREADER_H_
#define UTILITIES_CSVREADER_H_

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include "inet/common/INETDefs.h"

namespace utilities {

/**
 * Structure to hold 3D antenna radiation pattern data
 * Phi (azimuth): -180° to 180° (azimuthal angle)
 * Theta (elevation): -90° to 90° (angle from horizon)
 * Gain: dBi
 */
struct AntennaPattern {
    struct Point {
        double phi;      // Azimuth angle in degrees (-180° to 180°)
        double theta;    // Elevation angle in degrees (-90° to 90°)
        double gain;     // Gain in dBi
    };
    std::vector<Point> points;
    
    /**
     * Get interpolated gain for given phi and theta angles
     */
    double getGain(double phi, double theta) const;
};

/**
 * Structure to hold C/N0 to reception rate mapping
 * C/N0_dB: integer C/N0 value in dB
 * failure_rate: float 0-1 (probability of reception failure)
 */
struct CN0ReceptionMap {
    struct Entry {
        int cn0_dB;
        double failure_rate;  // 0-1 range
    };
    std::vector<Entry> entries;
    
    /**
     * Get failure rate for given C/N0 (with interpolation)
     */
    double getFailureRate(double cn0_dB) const;
};

/**
 * Structure to hold a 2D relative interference map (lon/lat grid)
 * Map represents interference levels at different geographic locations
 * Longitude: -180° to 180°, Latitude: -90° to 90°
 */
struct RelativeMap {
    struct Point {
        double longitude;      // -180° to 180°
        double latitude;       // -90° to 90°
        double value;          // Relative interference level (dB)
    };
    std::vector<double> longitudes;  // Sorted unique longitude values
    std::vector<double> latitudes;   // Sorted unique latitude values
    std::vector<std::vector<double>> grid;  // 2D grid: grid[lat_idx][lon_idx]
    
    /**
     * Get value at nearest grid point for given longitude and latitude
     */
    double getValue(double lon, double lat) const;
};

/**
 * CSV reader utility class for loading PHY layer parameters
 */
class CSVReader {
public:
    /**
     * Load 3D antenna radiation pattern from CSV file
     * Expected format: Phi,Theta,Gain
     * Where Phi is azimuth (-180° to 180°), Theta is elevation (-90° to 90°), Gain is in dBi
     */
    static AntennaPattern loadAntennaPattern(const std::string& filename);
    
    /**
     * Load C/N0 to reception rate mapping from CSV file
     * Expected format: C/N0_dB,failure_rate
     * Where C/N0_dB is integer, failure_rate is float 0-1
     */
    static CN0ReceptionMap loadCN0ReceptionMap(const std::string& filename);
    
    /**
     * Load relative interference map from CSV file
     * Expected format: 2D grid with lon/lat coordinates
     * First row: longitude values, First column: latitude values
     * Other cells: relative interference levels
     */
    static RelativeMap loadRelativeMap(const std::string& filename);
    
    /**
     * Load fixed noise power from parameter (for now, no CSV file)
     * Returns noise power in Watts
     */
    static double loadNoiseFloor(double noiseFloor_dBm);

private:
    /**
     * Helper function to trim whitespace from string
     */
    static std::string trim(const std::string& str);
    
    /**
     * Helper function to split CSV line
     */
    static std::vector<std::string> parseLine(const std::string& line, char delimiter = ',');
};

} // namespace utilities

#endif /* UTILITIES_CSVREADER_H_ */

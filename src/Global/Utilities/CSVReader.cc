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

#include "CSVReader.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include "inet/common/INETMath.h"

using namespace inet;

namespace utilities {

std::string CSVReader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> CSVReader::parseLine(const std::string& line, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(trim(item));
    }
    
    return result;
}

AntennaPattern CSVReader::loadAntennaPattern(const std::string& filename) {
    AntennaPattern pattern;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open antenna pattern file: " + filename);
    }
    
    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        line = trim(line);
        
        // Skip empty lines
        if (line.empty())
            continue;
            
        // Skip header line
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        std::vector<std::string> fields = parseLine(line);
        
        if (fields.size() != 3) {
            throw std::runtime_error("Invalid antenna pattern CSV format at line " + 
                                   std::to_string(lineNumber) + ": expected 3 fields (Phi,Theta,Gain)");
        }
        
        try {
            AntennaPattern::Point point;
            point.phi = std::stod(fields[0]);
            point.theta = std::stod(fields[1]);
            point.gain = std::stod(fields[2]);
            pattern.points.push_back(point);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing antenna pattern at line " + 
                                   std::to_string(lineNumber) + ": " + e.what());
        }
    }
    
    file.close();
    
    if (pattern.points.empty()) {
        throw std::runtime_error("No data points found in antenna pattern file: " + filename);
    }
    
    return pattern;
}

CN0ReceptionMap CSVReader::loadCN0ReceptionMap(const std::string& filename) {
    CN0ReceptionMap map;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open C/N0 reception map file: " + filename);
    }
    
    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        line = trim(line);
        
        // Skip empty lines
        if (line.empty())
            continue;
            
        // Skip header line
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        std::vector<std::string> fields = parseLine(line);
        
        if (fields.size() != 2) {
            throw std::runtime_error("Invalid C/N0 map CSV format at line " + 
                                   std::to_string(lineNumber) + ": expected 2 fields (C/N0_dB,failure_rate)");
        }
        
        try {
            CN0ReceptionMap::Entry entry;
            entry.cn0_dB = std::stoi(fields[0]);
            entry.failure_rate = std::stod(fields[1]);
            
            if (entry.failure_rate < 0.0 || entry.failure_rate > 1.0) {
                throw std::runtime_error("failure_rate must be in range [0,1]");
            }
            
            map.entries.push_back(entry);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing C/N0 map at line " + 
                                   std::to_string(lineNumber) + ": " + e.what());
        }
    }
    
    file.close();
    
    if (map.entries.empty()) {
        throw std::runtime_error("No data points found in C/N0 map file: " + filename);
    }
    
    // Sort entries by C/N0_dB for efficient lookup
    std::sort(map.entries.begin(), map.entries.end(),
              [](const CN0ReceptionMap::Entry& a, const CN0ReceptionMap::Entry& b) {
                  return a.cn0_dB < b.cn0_dB;
              });
    
    return map;
}



double CSVReader::loadNoiseFloor(double noiseFloor_dBm) {
    // Convert dBm to Watts
    return math::dBmW2mW(noiseFloor_dBm) / 1000.0;
}

double AntennaPattern::getGain(double phi, double theta) const {
    if (points.empty())
        return 0.0;
    
    EV << "AntennaPattern::getGain - Requested angles: phi=" << phi << "°, theta=" << theta << "°" << endl;
    
    // Normalize phi to -180 to 180 range
    while (phi > 180.0) phi -= 360.0;
    while (phi < -180.0) phi += 360.0;
    
    // Build a map of unique phi and theta values for grid determination
    std::set<double> uniquePhi, uniqueTheta;
    for (const auto& point : points) {
        uniquePhi.insert(point.phi);
        uniqueTheta.insert(point.theta);
    }
    
    // Convert to sorted vectors for binary search
    std::vector<double> phiValues(uniquePhi.begin(), uniquePhi.end());
    std::vector<double> thetaValues(uniqueTheta.begin(), uniqueTheta.end());
    
    // Find surrounding phi values
    auto phiUpper = std::lower_bound(phiValues.begin(), phiValues.end(), phi);
    auto phiLower = phiUpper;
    if (phiUpper != phiValues.begin()) {
        --phiLower;
    }
    
    // Find surrounding theta values
    auto thetaUpper = std::lower_bound(thetaValues.begin(), thetaValues.end(), theta);
    auto thetaLower = thetaUpper;
    if (thetaUpper != thetaValues.begin()) {
        --thetaLower;
    }
    
    // Handle edge cases
    if (phiUpper == phiValues.end()) {
        phiUpper = phiLower;
    }
    if (thetaUpper == thetaValues.end()) {
        thetaUpper = thetaLower;
    }
    
    double phi1 = *phiLower;
    double phi2 = *phiUpper;
    double theta1 = *thetaLower;
    double theta2 = *thetaUpper;
    
    // Helper lambda to find gain at specific phi/theta
    auto findGain = [this](double p, double t) -> double {
        for (const auto& point : points) {
            if (std::abs(point.phi - p) < 1e-6 && std::abs(point.theta - t) < 1e-6) {
                return point.gain;
            }
        }
        // If exact point not found, use nearest neighbor as fallback
        double minDist = 1e9;
        double bestGain = 0.0;
        for (const auto& point : points) {
            double phiDiff = std::abs(point.phi - p);
            if (phiDiff > 180.0) phiDiff = 360.0 - phiDiff;
            double thetaDiff = std::abs(point.theta - t);
            double dist = sqrt(phiDiff * phiDiff + thetaDiff * thetaDiff);
            if (dist < minDist) {
                minDist = dist;
                bestGain = point.gain;
            }
        }
        return bestGain;
    };
    
    // Get gain values at the four corners
    double g11 = findGain(phi1, theta1);
    double g12 = findGain(phi1, theta2);
    double g21 = findGain(phi2, theta1);
    double g22 = findGain(phi2, theta2);
    
    // If we're at a grid point, return exact value
    if (std::abs(phi1 - phi2) < 1e-6 && std::abs(theta1 - theta2) < 1e-6) {
        return g11;
    }
    
    // Bilinear interpolation
    double result;
    if (std::abs(phi1 - phi2) < 1e-6) {
        // Only interpolate in theta direction
        double t = (std::abs(theta2 - theta1) > 1e-6) ? (theta - theta1) / (theta2 - theta1) : 0.0;
        result = g11 * (1.0 - t) + g12 * t;
    } else if (std::abs(theta1 - theta2) < 1e-6) {
        // Only interpolate in phi direction
        double s = (phi - phi1) / (phi2 - phi1);
        result = g11 * (1.0 - s) + g21 * s;
    } else {
        // Full bilinear interpolation
        double s = (phi - phi1) / (phi2 - phi1);
        double t = (theta - theta1) / (theta2 - theta1);
        
        result = g11 * (1.0 - s) * (1.0 - t) +
                 g21 * s * (1.0 - t) +
                 g12 * (1.0 - s) * t +
                 g22 * s * t;
    }
    
    return result;
}

double CN0ReceptionMap::getFailureRate(double cn0_dB) const {
    if (entries.empty())
        return 1.0;  // If no data, assume failure
    
    // Handle out of bounds cases
    if (cn0_dB <= entries.front().cn0_dB)
        return entries.front().failure_rate;
    
    if (cn0_dB >= entries.back().cn0_dB)
        return entries.back().failure_rate;
    
    // Find surrounding entries for interpolation
    for (size_t i = 1; i < entries.size(); i++) {
        if (cn0_dB <= entries[i].cn0_dB) {
            // Linear interpolation
            double x0 = entries[i-1].cn0_dB;
            double y0 = entries[i-1].failure_rate;
            double x1 = entries[i].cn0_dB;
            double y1 = entries[i].failure_rate;
            
            double t = (cn0_dB - x0) / (x1 - x0);
            return y0 + t * (y1 - y0);
        }
    }
    
    return entries.back().failure_rate;
}

RelativeMap CSVReader::loadRelativeMap(const std::string& filename) {
    RelativeMap map;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open relative map file: " + filename);
    }
    
    std::string line;
    bool firstLine = true;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        line = trim(line);
        
        // Skip empty lines
        if (line.empty())
            continue;
        
        std::vector<std::string> fields = parseLine(line);
        
        if (firstLine) {
            firstLine = false;
            
            // First line contains longitude values (skip first column which is "Unnamed: 0" or similar)
            if (fields.size() < 2) {
                throw std::runtime_error("Invalid relative map CSV format: first row must contain longitude values");
            }
            
            // Parse longitude values starting from second column
            for (size_t i = 1; i < fields.size(); i++) {
                try {
                    map.longitudes.push_back(std::stod(fields[i]));
                } catch (const std::exception& e) {
                    throw std::runtime_error("Error parsing longitude at position " + 
                                           std::to_string(i) + " in first row: " + e.what());
                }
            }
            continue;
        }
        
        // Subsequent lines: first column is latitude, rest are values
        if (fields.size() != map.longitudes.size() + 1) {
            throw std::runtime_error("Invalid relative map CSV format at line " + 
                                   std::to_string(lineNumber) + ": expected " + 
                                   std::to_string(map.longitudes.size() + 1) + " columns, got " +
                                   std::to_string(fields.size()));
        }
        
        try {
            // First column is latitude
            double latitude = std::stod(fields[0]);
            map.latitudes.push_back(latitude);
            
            // Rest are the grid values
            std::vector<double> row;
            for (size_t i = 1; i < fields.size(); i++) {
                row.push_back(std::stod(fields[i]));
            }
            map.grid.push_back(row);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing relative map at line " + 
                                   std::to_string(lineNumber) + ": " + e.what());
        }
    }
    
    file.close();
    
    if (map.grid.empty() || map.longitudes.empty()) {
        throw std::runtime_error("No data points found in relative map file: " + filename);
    }
    
    return map;
}

double RelativeMap::getValue(double lon, double lat) const {
    if (grid.empty() || longitudes.empty() || latitudes.empty())
        return 0.0;
    
    // Normalize longitude to -180 to 180 range
    while (lon > 180.0) lon -= 360.0;
    while (lon < -180.0) lon += 360.0;
    
    // Clamp latitude to valid range
    if (lat > 90.0) lat = 90.0;
    if (lat < -90.0) lat = -90.0;
    
    // Find nearest longitude
    double minLonDist = 1e9;
    size_t nearestLonIdx = 0;
    for (size_t i = 0; i < longitudes.size(); i++) {
        double lonDiff = std::abs(longitudes[i] - lon);
        // Account for wraparound at dateline
        if (lonDiff > 180.0) lonDiff = 360.0 - lonDiff;
        
        if (lonDiff < minLonDist) {
            minLonDist = lonDiff;
            nearestLonIdx = i;
        }
    }
    
    // Find nearest latitude
    double minLatDist = 1e9;
    size_t nearestLatIdx = 0;
    for (size_t i = 0; i < latitudes.size(); i++) {
        double latDiff = std::abs(latitudes[i] - lat);
        if (latDiff < minLatDist) {
            minLatDist = latDiff;
            nearestLatIdx = i;
        }
    }
    
    // Return value at nearest grid point
    if (nearestLatIdx < grid.size() && nearestLonIdx < grid[nearestLatIdx].size()) {
        return grid[nearestLatIdx][nearestLonIdx];
    }
    
    return 0.0;
}


} // namespace utilities

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

#include "CustomPHYPathLoss.h"

namespace channel {

Define_Module(CustomPHYPathLoss);

CustomPHYPathLoss::CustomPHYPathLoss()
{
}

void CustomPHYPathLoss::initialize(int stage)
{
    FreeSpacePathLoss::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        polarizationLoss_dB = par("polarizationLoss");
        fadingSigma_dB = par("fadingSigma");
        decodingImplementation_dB = par("decodingImplementation");
        noiseFigureEnhancement_dB = par("noiseFigureEnhancement");
        pathLossVector = new cOutVector("Path Loss (dB)");
        fadingLawVector = new cOutVector("Fading Loss (dB)");
    }
}

void CustomPHYPathLoss::finish()
{
    delete pathLossVector;
    delete fadingLawVector;
}

double CustomPHYPathLoss::computePathLoss(mps propagationSpeed, Hz frequency, m distance) const
{
    // Compute free space path loss using parent class
    double freeSpacePathLoss = FreeSpacePathLoss::computePathLoss(propagationSpeed, frequency, distance);
    
    // Convert to dB for adding additional losses
    double pathLoss_dB = 10.0 * log10(freeSpacePathLoss);
    
    // Add polarization loss
    pathLoss_dB -= polarizationLoss_dB;
    
    // Add log-normal fading (random variable with mean 0 and sigma fadingSigma_dB)
    double fading_dB = normal(0.0, fadingSigma_dB);
    pathLoss_dB -= fading_dB;
    
    // Record fading components
    // fadingLawVector->record(fading_dB);
    // Print the length of the fading vector for debugging
    // EV << "Fading Vector Length " << fadingLawVector->getValuesReceived() << " dB" << endl;

    // Add decoding implementation
    pathLoss_dB -= decodingImplementation_dB;

    // Add noise figure enchancement
    pathLoss_dB -= noiseFigureEnhancement_dB;

    EV << "CustomPHYPathLoss: FSPL=" << 10.0 * log10(freeSpacePathLoss) << " dB"
        << ", Polarization=" << polarizationLoss_dB << " dB"
        << ", Fading=" << fading_dB << " dB"
        << ", RxSysLoss=" << decodingImplementation_dB << " dB"
        << ", NoiseFigEnh=" << noiseFigureEnhancement_dB << " dB"
        << ", Total=" << pathLoss_dB << " dB" << endl;
    
    // pathLossVector->record(pathLoss_dB);
    // Convert back to linear scale
    return math::dB2fraction(pathLoss_dB);
}

} // namespace channel

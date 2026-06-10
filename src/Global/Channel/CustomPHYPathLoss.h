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

#ifndef CUSTOMPHYPATHLOSS_H_
#define CUSTOMPHYPATHLOSS_H_

#include "inet/physicallayer/wireless/common/pathloss/FreeSpacePathLoss.h"

using namespace inet;
using namespace inet::physicallayer;

namespace channel {

/**
 * This class implements a path loss model for satellite communications.
 * It extends the free space path loss with additional losses:
 * - Polarization loss
 * - Log-normal fading
 * - Receiver system loss
 */
class CustomPHYPathLoss : public FreeSpacePathLoss
{
  protected:
    // Vector recorder 
    cOutVector *pathLossVector;
    cOutVector *fadingLawVector;
    
    virtual void finish() override;

    // Polarization loss in dB (typically 3 dB for circular-linear mismatch)
    double polarizationLoss_dB;
    
    // Log-normal fading standard deviation in dB
    double fadingSigma_dB;
    
    // Receiver system loss in dB (cable losses, connector losses, etc.)
    double decodingImplementation_dB;

    // Additional noise figure enhancement in dB
    double noiseFigureEnhancement_dB;

  protected:
    virtual void initialize(int stage) override;

  public:
    CustomPHYPathLoss();
    virtual double computePathLoss(mps propagationSpeed, Hz frequency, m distance) const override;
};

} // namespace channel

#endif /* CUSTOMPHYPATHLOSS_H_ */

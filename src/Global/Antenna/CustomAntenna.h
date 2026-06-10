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

#ifndef CUSTOMANTENNA_H_
#define CUSTOMANTENNA_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/AntennaBase.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IAntennaGain.h"
#include "Global/Utilities/CSVReader.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace utilities;

namespace antenna {

/**
 * Custom antenna that loads 3D radiation patterns from CSV files
 * CSV format: Phi (azimuth), Theta (elevation), Gain (dBi)
 * Phi: -180° to 180° (azimuth angle)
 * Theta: -90° to 90° (elevation angle from horizon)
 */
class CustomAntenna : public AntennaBase
{
protected:
    class AntennaGain : public IAntennaGain {
    protected:
        AntennaPattern pattern;
        double maxGain_dBi;
        double minGain_dBi;

    public:
        AntennaGain(const AntennaPattern& pattern);
        
        virtual double getMinGain() const override;
        virtual double getMaxGain() const override;
        virtual double computeGain(const Quaternion& direction) const override;
        virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
    };

protected:
    AntennaPattern antennaPattern;
    Ptr<AntennaGain> gain;

protected:
    virtual void initialize(int stage) override;

public:
    virtual Ptr<const IAntennaGain> getGain() const override { return gain; }
    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
};

} // namespace antenna

#endif /* CUSTOMANTENNA_H_ */

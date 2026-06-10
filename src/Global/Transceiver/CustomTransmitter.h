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

#ifndef CUSTOMTRANSMITTER_H_
#define CUSTOMTRANSMITTER_H_

#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"
#include "Global/Channel/LoRaModulation.h"
#include "CustomTransmission.h"
#include "Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.h"
#include "Global/Utilities/libnorad/ccoord.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"

using namespace mobility;
using namespace inet::physicallayer;

namespace transceiver {

/**
 * Custom transmitter for CSV-based PHY layer
 * Removes LoRa-specific logic (SF, BW, CR parameters)
 * Uses fixed frequency 401.63 MHz for satellite communications
 */
class CustomTransmitter : public FlatTransmitterBase
{
public:
    CustomTransmitter();
    virtual void initialize(int stage) override;
    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
    virtual const ITransmission *createTransmission(const IRadio *radio, const Packet *packet, const simtime_t startTime) const override;

private:
    bool iAmGateway;
    simsignal_t transmissionCreatedSignal;
};

} // namespace transceiver

#endif /* CUSTOMTRANSMITTER_H_ */

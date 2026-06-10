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
#ifndef LORAPHY_LORAMEDIUM_H_
#define LORAPHY_LORAMEDIUM_H_
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.h"
#include "Global/Utilities/Utils.h"

#include "inet/common/IntervalTree.h"
#include "inet/environment/contract/IMaterialRegistry.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/physicallayer/wireless/common/medium/CommunicationLog.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/Radio.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/ICommunicationCache.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IMediumLimitCache.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/INeighborCache.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include <algorithm>

#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/queueing/base/ActivePacketSinkBase.h"

using namespace omnetpp;
using namespace inet;

namespace medium {
class LoRaMedium : public RadioMedium
{
    friend class LoRaGWRadio;
    friend class LoRaRadio;

protected:
    int mapX, mapY;
    double minimumElevationTermSat = 0.0;
    virtual bool matchesMacAddressFilter(const IRadio *radio, const Packet *packet) const override;
    virtual bool isInCommunicationRange(const ITransmission *transmission, const Coord& startPosition, const Coord& endPosition) const override;
    virtual bool isPotentialReceiver(const IRadio *receiver, const ITransmission *transmission) const override;
        //@}
    public:
      mutable const IRadio* globalPotentialReceiver = nullptr;
      LoRaMedium();
      virtual ~LoRaMedium();
      virtual void initialize(int stage) override;
      //virtual const IReceptionDecision *getReceptionDecision(const IRadio *receiver, const IListening *listening, const ITransmission *transmission, IRadioSignal::SignalPart part) const override;
      virtual const IReceptionResult *getReceptionResult(const IRadio *receiver, const IListening *listening, const ITransmission *transmission) const override;
      virtual void addTransmission(const IRadio *transmitterRadio, const ITransmission *transmission) override;
      virtual IWirelessSignal *createTransmitterSignal(const IRadio *radio, Packet *packet) override;

};
}
#endif /* LORAPHY_LORAMEDIUM_H_ */

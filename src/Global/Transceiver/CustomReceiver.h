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

#ifndef CUSTOMRECEIVER_H_
#define CUSTOMRECEIVER_H_

#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionResult.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/BandListening.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ListeningDecision.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/NarrowbandNoiseBase.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/ScalarSnir.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h"
#include "Global/Transceiver/CustomTransmission.h"
#include "Global/Transceiver/CustomReception.h"
#include "Global/Channel/CustomBandListening.h"
#include "Global/Utilities/CSVReader.h"
#include "Global/Base/_20_Data_Link/Base_MacProtocol.h"
#include "Global/Messages/_20_Data_Link/Base_MacFrame_m.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"

using namespace utilities;

namespace transceiver {

/**
 * Custom receiver for CSV-based PHY layer with C/N0-based decision logic
 * Removes LoRa-specific collision detection (SF matrices)
 * Uses CSV-loaded C/N0-to-reception-rate mappings
 */
class CustomReceiver : public FlatReceiverBase
{
protected:
   

private:
    Hz centerFrequency;
    Hz bandwidth;
    W transmissionPower;
    
    double snirThreshold;
    bool iAmGateway;
    int satIndex;
    
    W energyDetection;
    simsignal_t receptionCollisionSignal;
    simsignal_t belowSensitivitySignal;
    
    // CSV-loaded data
    RelativeMap relativeMapI0;
    std::string relativeMapFile;

    CN0ReceptionMap cn0ReceptionMap;
    std::string cn0MapFile;

    
    double noiseFloor_dBm;
    
    // Statistics
    long numCollisions;
    long rcvBelowSensitivity;
    long numCN0Failures;

    cOutVector *receivedPowerVector;
    cOutVector *receivedCN0Vector;
    cOutVector *expectedCN0Vector;
    cOutVector *relativeInterferenceVector;
    cOutVector *positionWhenInterferenceIsComputedVector;
    
public:
    CustomReceiver();
    
    void initialize(int stage) override;
    void finish() override;
    
    virtual W getMinInterferencePower() const override { return W(NaN); }
    virtual W getMinReceptionPower() const override { return W(NaN); }
    
    virtual bool computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const override;
    virtual bool computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const override;
    virtual bool computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const override;
    
    virtual Packet * computeReceivedPacket(const ISnir *snir, bool isReceptionSuccessful) const override;
    
    virtual const IReceptionDecision *computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const override;
    virtual const IReceptionResult *computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const override;
    
    virtual bool computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const override;
    
    virtual double getSNIRThreshold() const override { return snirThreshold; }
    virtual const IListening *createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord& startPosition, const Coord& endPosition) const override;
    
    virtual const IListeningDecision *computeListeningDecision(const IListening *listening, const IInterference *interference) const override;
    
    /**
     * Determine reception success based on C/N0 and CSV mapping
     * Returns true if reception succeeds (stochastically)
     */
    bool decideCN0Reception(double cn0_dB) const;
};

} // namespace transceiver

#endif /* CUSTOMRECEIVER_H_ */

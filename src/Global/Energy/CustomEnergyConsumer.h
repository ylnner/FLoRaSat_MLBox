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

#ifndef CUSTOMENERGYCONSUMER_H_
#define CUSTOMENERGYCONSUMER_H_

#include "inet/physicallayer/wireless/common/energyconsumer/StateBasedEpEnergyConsumer.h"
// #include "inet/power/storage/IdealEpEnergyStorage.h"
#include <map>
#include "inet/common/ModuleAccess.h"

using namespace inet;
using namespace inet::physicallayer;

/**
 * Generic energy consumer that records time spent in each radio state.
 * This allows post-processing with different device power parameters
 * without re-running simulations.
 */
class CustomEnergyConsumer: public inet::physicallayer::StateBasedEpEnergyConsumer {
public:
    using omnetpp::cIListener::finish; // avoid hiding cIListener::finish(component, signalID)
    void initialize(int stage) override;
    void finish() override;
    virtual W getPowerConsumption() const override;
    virtual void receiveSignal(cComponent *source, simsignal_t signal, intval_t value, cObject *details) override;

protected:
    // Radio state tracking
    struct RadioState {
        simtime_t lastChangeTime;
        simtime_t totalTime;
        
        RadioState() : lastChangeTime(0), totalTime(0) {}
    };
    
    // Time tracking for different radio states
    RadioState offState;
    RadioState sleepState;
    RadioState switchingState;
    RadioState idleState;
    RadioState receivingState;
    RadioState transmittingState;
    RadioState busyState;
    
    // Current state tracking
    simtime_t lastStateUpdateTime;
    
    // Track current radio mode and states
    IRadio::RadioMode currentRadioMode;
    IRadio::ReceptionState currentReceptionState;
    IRadio::TransmissionState currentTransmissionState;
    
    // Helper methods
    void updateStateTime();
    void recordCurrentState();
    std::string getStateString() const;
};
#endif /* CUSTOMENERGYCONSUMER_H_ */

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

#include "CustomEnergyConsumer.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"

using namespace inet::power;

Define_Module(CustomEnergyConsumer);

void CustomEnergyConsumer::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // --------------------------------------------------------------------
        // Initialize all power consumption values to zero
        // (this module tracks time spent in states, not absolute power values)
        // --------------------------------------------------------------------
        offPowerConsumption = W(0);
        sleepPowerConsumption = W(0);
        switchingPowerConsumption = W(0);
        receiverIdlePowerConsumption = W(0);
        transmitterIdlePowerConsumption = W(0);
        receiverReceivingPowerConsumption = W(0);
        receiverBusyPowerConsumption = W(0);
        transmitterTransmittingPowerConsumption = W(0);
        transmitterTransmittingPreamblePowerConsumption = W(0);
        transmitterTransmittingHeaderPowerConsumption = W(0);
        transmitterTransmittingDataPowerConsumption = W(0);

        // --------------------------------------------------------------------
        // Initialize state tracking
        // --------------------------------------------------------------------
        lastStateUpdateTime = simTime();
        currentRadioMode = IRadio::RADIO_MODE_OFF;
        currentReceptionState = IRadio::RECEPTION_STATE_UNDEFINED;
        currentTransmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;

        // --------------------------------------------------------------------
        // Subscribe to radio state change signals
        // --------------------------------------------------------------------
        cModule *radioModule = getParentModule();
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receivedSignalPartChangedSignal, this);
        radioModule->subscribe(IRadio::transmittedSignalPartChangedSignal, this);

        // --------------------------------------------------------------------
        // INET 4.3 (legacy):
        // The energy source was stored as a raw IdealEpEnergyStorage pointer.
        //
        // radio = check_and_cast<IRadio *>(radioModule);
        // const char *energySourceModule = par("energySourceModule");
        // energySource = dynamic_cast<IdealEpEnergyStorage *>(
        //     getParentModule()->getSubmodule(energySourceModule));
        // if (!energySource)
        //     throw cRuntimeError("Cannot find power source");
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        // INET 4.5 adaptation:
        // StateBasedEpEnergyConsumer now stores the energy source as
        // ModuleRefByPar<IEpEnergySource>. The energy source is resolved
        // via the NED/INI parameter "energySourceModule" instead of being
        // assigned as a raw cModule pointer.
        // --------------------------------------------------------------------
        radio = check_and_cast<IRadio *>(radioModule);
        energySource.reference(this, "energySourceModule", true);
        if (energySource.get() == nullptr)
            throw cRuntimeError("Cannot find power source (energySourceModule=%s)",
                                par("energySourceModule").stringValue());
    }
    else if (stage == INITSTAGE_POWER) {
        // --------------------------------------------------------------------
        // INET 4.3 (legacy):
        // energySource->addEnergyConsumer(this);
        //
        // INET 4.5 adaptation:
        // Access the resolved energy source through ModuleRefByPar.
        // --------------------------------------------------------------------
        energySource.get()->addEnergyConsumer(this);
    }
}

void CustomEnergyConsumer::finish()
{
    // Update final state time before recording
    updateStateTime();
    
    // Record total time in each state
    recordScalar("timeInOffState", offState.totalTime.dbl());
    recordScalar("timeInSleepState", sleepState.totalTime.dbl());
    recordScalar("timeInSwitchingState", switchingState.totalTime.dbl());
    recordScalar("timeInIdleState", idleState.totalTime.dbl());
    recordScalar("timeInReceivingState", receivingState.totalTime.dbl());
    recordScalar("timeInTransmittingState", transmittingState.totalTime.dbl());
    recordScalar("timeInBusyState", busyState.totalTime.dbl());
    
    // Record total simulation time for verification
    simtime_t totalTime = offState.totalTime + sleepState.totalTime + switchingState.totalTime +
                          idleState.totalTime + receivingState.totalTime + 
                          transmittingState.totalTime + busyState.totalTime;
    recordScalar("totalTrackedTime", totalTime.dbl());
    
    // Log state distribution for easy verification
    EV_INFO << "CustomEnergyConsumer - Time in states:\n";
    EV_INFO << "  Off: " << offState.totalTime << "s\n";
    EV_INFO << "  Sleep: " << sleepState.totalTime << "s\n";
    EV_INFO << "  Switching: " << switchingState.totalTime << "s\n";
    EV_INFO << "  Idle: " << idleState.totalTime << "s\n";
    EV_INFO << "  Receiving: " << receivingState.totalTime << "s\n";
    EV_INFO << "  Transmitting: " << transmittingState.totalTime << "s\n";
    EV_INFO << "  Busy: " << busyState.totalTime << "s\n";
    EV_INFO << "  Total: " << totalTime << "s\n";
}

void CustomEnergyConsumer::receiveSignal(cComponent *source, simsignal_t signal, intval_t value, cObject *details)
{
    if (signal == IRadio::radioModeChangedSignal ||
        signal == IRadio::receptionStateChangedSignal ||
        signal == IRadio::transmissionStateChangedSignal ||
        signal == IRadio::receivedSignalPartChangedSignal ||
        signal == IRadio::transmittedSignalPartChangedSignal)
    {
        // Update time for previous state before switching
        updateStateTime();
        recordCurrentState();
        
        // Update current states
        currentRadioMode = radio->getRadioMode();
        currentReceptionState = radio->getReceptionState();
        currentTransmissionState = radio->getTransmissionState();
        
        // Update last change time
        lastStateUpdateTime = simTime();
        
        // Always report zero power consumption (we only track time)
        powerConsumption = W(0);
        emit(powerConsumptionChangedSignal, powerConsumption.get());
    }
    else
        throw cRuntimeError("Unknown signal");
}

W CustomEnergyConsumer::getPowerConsumption() const
{
    // This module only tracks time, not actual power consumption
    // Return 0 to not affect energy storage calculations
    return W(0);
}

void CustomEnergyConsumer::updateStateTime()
{
    simtime_t currentTime = simTime();
    simtime_t timeDelta = currentTime - lastStateUpdateTime;
    
    if (timeDelta <= 0)
        return;
    
    // Determine which state we were in and update its time
    if (currentRadioMode == IRadio::RADIO_MODE_OFF) {
        offState.totalTime += timeDelta;
    }
    else if (currentRadioMode == IRadio::RADIO_MODE_SLEEP) {
        sleepState.totalTime += timeDelta;
    }
    else if (currentRadioMode == IRadio::RADIO_MODE_SWITCHING) {
        switchingState.totalTime += timeDelta;
    }
    else if (currentRadioMode == IRadio::RADIO_MODE_RECEIVER || 
             currentRadioMode == IRadio::RADIO_MODE_TRANSCEIVER ||
             currentRadioMode == IRadio::RADIO_MODE_TRANSMITTER) {
        
        // Check if we're transmitting
        if (currentTransmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING) {
            transmittingState.totalTime += timeDelta;
        }
        // Check if we're receiving
        else if (currentReceptionState == IRadio::RECEPTION_STATE_RECEIVING) {
            receivingState.totalTime += timeDelta;
        }
        // Check if receiver is busy (interference/listening)
        else if (currentReceptionState == IRadio::RECEPTION_STATE_BUSY) {
            busyState.totalTime += timeDelta;
        }
        // Otherwise we're idle
        else if (currentReceptionState == IRadio::RECEPTION_STATE_IDLE ||
                 currentTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            idleState.totalTime += timeDelta;
        }
    }
}

void CustomEnergyConsumer::recordCurrentState()
{
    // Record the current state for debugging purposes
    std::string state = getStateString();
    EV_INFO << "Radio state: " << state << " at " << simTime() << endl;
}

std::string CustomEnergyConsumer::getStateString() const
{
    std::string result = "RadioMode=";
    
    switch (currentRadioMode) {
        case IRadio::RADIO_MODE_OFF:
            result += "OFF";
            break;
        case IRadio::RADIO_MODE_SLEEP:
            result += "SLEEP";
            break;
        case IRadio::RADIO_MODE_SWITCHING:
            result += "SWITCHING";
            break;
        case IRadio::RADIO_MODE_RECEIVER:
            result += "RECEIVER";
            break;
        case IRadio::RADIO_MODE_TRANSMITTER:
            result += "TRANSMITTER";
            break;
        case IRadio::RADIO_MODE_TRANSCEIVER:
            result += "TRANSCEIVER";
            break;
        default:
            result += "UNKNOWN";
    }
    
    result += ", ReceptionState=";
    switch (currentReceptionState) {
        case IRadio::RECEPTION_STATE_UNDEFINED:
            result += "UNDEFINED";
            break;
        case IRadio::RECEPTION_STATE_IDLE:
            result += "IDLE";
            break;
        case IRadio::RECEPTION_STATE_BUSY:
            result += "BUSY";
            break;
        case IRadio::RECEPTION_STATE_RECEIVING:
            result += "RECEIVING";
            break;
        default:
            result += "UNKNOWN";
    }
    
    result += ", TransmissionState=";
    switch (currentTransmissionState) {
        case IRadio::TRANSMISSION_STATE_UNDEFINED:
            result += "UNDEFINED";
            break;
        case IRadio::TRANSMISSION_STATE_IDLE:
            result += "IDLE";
            break;
        case IRadio::TRANSMISSION_STATE_TRANSMITTING:
            result += "TRANSMITTING";
            break;
        default:
            result += "UNKNOWN";
    }
    
    return result;
}
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

package Global.Energy;

import inet.physicallayer.wireless.common.energyconsumer.StateBasedEpEnergyConsumer;

//
// Generic energy consumer that records time spent in each radio state.
// This module does not calculate actual energy consumption, instead it tracks
// the time spent in different radio states (off, sleep, idle, receiving, 
// transmitting, busy, switching). This allows post-processing with different
// device power parameters without re-running simulations.
//
// The module records the following statistics:
//  - timeInOffState: Total time in OFF state
//  - timeInSleepState: Total time in SLEEP state
//  - timeInSwitchingState: Total time in SWITCHING state
//  - timeInIdleState: Total time in IDLE state (radio on but not tx/rx)
//  - timeInReceivingState: Total time actively receiving
//  - timeInTransmittingState: Total time actively transmitting
//  - timeInBusyState: Total time with receiver busy (interference)
//  - totalTrackedTime: Sum of all state times (for verification)
//
// To calculate energy consumption in post-processing:
//   Energy = timeInState_i * powerConsumption_i for each state i
//
simple CustomEnergyConsumer extends StateBasedEpEnergyConsumer
{
    parameters:
        @class(CustomEnergyConsumer);
        @display("i=block/cogwheel");
        
        // No configuration file needed - this module only tracks time
        // All power consumption parameters are set to 0 internally
}

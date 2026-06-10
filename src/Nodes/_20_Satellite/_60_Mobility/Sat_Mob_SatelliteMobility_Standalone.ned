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

package Nodes._20_Satellite._60_Mobility;

import inet.mobility.base.MovingMobilityBase;
import Global.Mobility.Base_IMobility;

//module Sat_Mob_SatelliteMobility extends MobilityBase
simple Sat_Mob_SatelliteMobility_Standalone extends MovingMobilityBase like Base_IMobility
{
    parameters:        
        //@class("mobility::sat_mob");
        @class(mobility::Sat_Mob_SatelliteMobility_Standalone);
        @display("i=block/cogwheel");
        //bool faceForward = default(true);
		bool displaySpanArea = default(false);
        int effectiveSlant = default(0);
}

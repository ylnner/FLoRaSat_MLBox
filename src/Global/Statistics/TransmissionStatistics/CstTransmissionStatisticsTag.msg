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

import inet.common.TagBase;

namespace statistics;


class CstTransmissionStatisticsTag extends inet::TagBase {    
    
    int packetId	= -1;
    double latDev	= 0;
    double longDev 	= 0;
    int satId 		= -1;
    double elevSat 	= 0;
    double doppler 	= 0;
    
    double loraTP	= 0;
    double loraCF	= 0;
    int loraSF		= 0;
    double loraBW	= 0;
    int loraCR		= 0;
    
	int rcvOk		= -1;
    
    double txStart = 0;
    double txDuration = -1;
    int repetition = 0;
    
    //rssi_dbm, snr_db, rx_power_dbm, collision_flag, num_colliding_packets    
    double rssi_dbm = 0;
    double snr_db = 0;
    double rx_power_dbm = 0;
    double noise_dbm = 0;
    int total_interferences = -1;
    int collision_flag = 0;
    int num_colliding_packets = -1;
    
    double minPower_dbm = 0;
    double sensitivity_dbm = 0;
   	
   	//This value is calculated at the device, not in the satellite. 
    double rx_power_dbm_dev = 0;
}
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
    
    
}
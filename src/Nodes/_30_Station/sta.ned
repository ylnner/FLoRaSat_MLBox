package Nodes._30_Station;

import Global.Utilities.LayerForwarder;

import Nodes._30_Station._10_Physical.Sta_Phy;
import Nodes._30_Station._20_Data_Link.Sta_Dlk;
import Nodes._30_Station._30_Network.Sta_Net;
import Nodes._30_Station._40_Application.StationApplicationLayer;
import Nodes._30_Station._50_Energy.StationEnergy;
import Nodes._30_Station._60_Mobility.Sta_Mob_StationMobility;

module Station extends LayerForwarder
{
	parameters:
		@display("i=device/receiverdish");
		@class(station::Sta);
		layerName = default("Station");
		
		bool sendData @mutable = default(false);
        int dstGsOrDev = default(1);
	gates:
		
		inout ethg @labels(EtherFrame-conn) @allowUnconnected;
        inout satelliteLink[60] @labels(EtherFrame-conn);
	submodules:
		phy: Sta_Phy {
			@display("p=100,260");
		}
		dlk: Sta_Dlk {
			@display("p=100,190");
		}
		net: Sta_Net {
			@display("p=100,120");
		}
		app: StationApplicationLayer {
			@display("p=100,50");
		}
		energy: StationEnergy {
			@display("p=230,170");
		}
		mobility: Sta_Mob_StationMobility {
			@display("p=230,230");
		}
	connections allowunconnected:
		app.lowerOut --> net.upperIn;
		net.upperOut --> app.lowerIn;

		net.lowerOut --> dlk.upperIn;
		dlk.upperOut --> net.lowerIn;
		
		phy.upperOut --> dlk.lowerIn;
		
		for i=0..sizeof(satelliteLink)-1 {
			satelliteLink[i] <--> phy.satLink[i];
		}
}

package Nodes._40_Server;

import Global.Utilities.LayerForwarder;

import Nodes._40_Server._10_Physical.ServerPhysicalLayer;
import Nodes._40_Server._20_Data_Link.ServerDataLinkLayer;
import Nodes._40_Server._30_Network.ServerNetworkLayer;
import Nodes._40_Server._40_Application.ServerApplicationLayer;
import Nodes._40_Server._50_Energy.ServerEnergy;
import Nodes._40_Server._60_Mobility.ServerMobility;

module Server extends LayerForwarder
{
    parameters:
        @display("i=device/server");
    gates:
        input accessIn;
        output accessOut;
    submodules:
        phy: ServerPhysicalLayer {
            @display("p=100,260;i=block/tunnel");
        }
        dlk: ServerDataLinkLayer {
            @display("p=100,190");
        }
        net: ServerNetworkLayer {
            @display("p=100,120");
        }
        app: ServerApplicationLayer {
            @display("p=100,50");
        }
        energy: ServerEnergy {
            @display("p=230,170;i=block/control");
        }
        mobility: ServerMobility {
            @display("p=230,230");
        }

    connections allowunconnected:
        app.lowerOut --> net.upperIn;
        net.upperOut --> app.lowerIn;

        net.lowerOut --> dlk.upperIn;
        dlk.upperOut --> net.lowerIn;

        dlk.lowerOut --> phy.upperIn;
        phy.upperOut --> dlk.lowerIn;

        accessIn --> phy.lowerIn;
        phy.lowerOut --> accessOut;

}

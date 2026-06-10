package Nodes._10_Terminal;

import Global.Utilities.LayerForwarder;

import Nodes._10_Terminal._10_Physical.Ter_Phy;
import Nodes._10_Terminal._20_Data_Link.Ter_Dlk;
import Nodes._10_Terminal._30_Network.Ter_Net;
import Nodes._10_Terminal._40_Application.Ter_App;
import Nodes._10_Terminal._50_Energy.Ter_Ene;
import Nodes._10_Terminal._60_Mobility.Ter_Mob;
import Nodes._10_Terminal._20_Data_Link.Ter_Dlk_KiWan;

module Terminal extends LayerForwarder
{
    parameters:
        @networkNode();
        @display("bgb=269.43335,304.14;i=device/accesspoint,gold,90;is=s");
        @class(terminal::Ter);
        layerName = default("Terminal");
        bool sendData @mutable = default(true);
        int dstGsOrDev = default(0);

    gates:
        input radioIn;
        output radioOut;
        
    submodules:
        phy: Ter_Phy {
            @display("p=100,250");
        }
        dlk: Ter_Dlk {
            @display("p=100,180");
        }
        net: Ter_Net {
            @display("p=100,110");
        }
        app: Ter_App {
            @display("p=100,40");
        }
        eng: Ter_Ene {
            @display("p=240,160");
        }
        mob: Ter_Mob {
            @display("p=240,220");
        }
        
    connections allowunconnected:
        dlk.lowerOut --> phy.upperIn;
        phy.upperOut --> dlk.lowerIn;

        net.lowerOut --> dlk.upperIn;
        dlk.upperOut --> net.lowerIn;

        app.appOut --> net.upperIn;
        net.upperOut --> app.appIn;
}

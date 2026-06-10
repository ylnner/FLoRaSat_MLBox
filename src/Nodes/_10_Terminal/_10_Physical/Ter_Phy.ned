package Nodes._10_Terminal._10_Physical;

import Global.Utilities.LayerForwarder;
import Nodes._10_Terminal._10_Physical.Ter_Phy_CustomRadio;
import Nodes._10_Terminal._10_Physical.Ter_Phy_LoRaRadio;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadio;

module Ter_Phy extends LayerForwarder
{
    parameters:
        layerName = default("Ter_Phy");
        @display("i=block/tunnel");

    gates:
        input radioIn;
        output radioOut;

    submodules:
        radio: <default("Ter_Phy_LoRaRadio")> like IRadio {
            @display("p=100,100;i=device/antenna");
        }

    connections allowunconnected:
        upperIn --> radio.upperLayerIn;
        radio.upperLayerOut --> upperOut;
}

package Nodes._20_Satellite.DSL._10_Physical;

import Global.Utilities.LayerForwarder;
import Nodes._20_Satellite.DSL._10_Physical.Sat_Dsl_Phy_CustomRadio;
import Nodes._20_Satellite.DSL._10_Physical.Sat_Dsl_Phy_LoRaRadio;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadio;

module Sat_Dsl_Phy extends LayerForwarder
{
    parameters:
        layerName = default("Sat_Dsl_Phy");
        @display("i=block/tunnel");

    submodules:
        radio: <default("Sat_Dsl_Phy_LoRaRadio")> like IRadio {
            @display("p=100,100;i=device/antenna");
        }

    connections allowunconnected:
        upperIn --> radio.upperLayerIn;
        radio.upperLayerOut --> upperOut;
}

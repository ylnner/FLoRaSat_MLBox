package Nodes._20_Satellite.DSL._20_Data_Link;

import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;

module Sat_Dsl_Dlk_kMac extends MacProtocolBase like IMacProtocol
{
    parameters:
        @class(mac::Sat_Dsl_Dlk_kMac);
        string address @mutable = default("auto");  // MAC address
        int headerLength @unit(B) = default(52B);   // length of the MAC header
        int mtu = default(1500);                    // Maximum Transmission Unit
        int satIndex = default(0);                  // Index of the satellite in the constellation

        // LoRa parameters
        int LoRaSF = default(10);
        double LoRaTP = default(25);
        double LoRaCF= default(868000000);
        int LoRaBW = default(125000);
        int LoRaCR = default(1);

        // Phy fixed loss 
        double fixedLossRate = default(0);          // Fixed loss in percentage
        
    connections allowunconnected:
}

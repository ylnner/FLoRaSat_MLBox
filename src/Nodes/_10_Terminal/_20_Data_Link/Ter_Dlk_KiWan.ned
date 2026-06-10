package Nodes._10_Terminal._20_Data_Link;

import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;
import inet.queueing.contract.IPacketQueue;

import Nodes._10_Terminal._20_Data_Link.Transmission_Predictor.Transmission_Predictor;

module Ter_Dlk_KiWan extends MacProtocolBase like IMacProtocol
{
    parameters:
        @class(mac::Ter_Dlk_KiWan);

        // Radio configuration
        string radioModule = default("^.^.phy.radio");        // Path to radio module
        string address @mutable = default("auto");      // MAC address, 'auto' for auto-assigned
        int mtu = default(1500);                        // Maximum Transmission Unit

        // Mac parameters
        double ttwDuration @unit(s) = default(900s);    // Duration of transmission window
        double ttwIdle @unit(s) = default(0s);          // Idle time between transmission windows
        double ttxStart_min @unit(s) = default(0s);     // Minimum time to start first transmission
        double ttxStart_max @unit(s) = default(60s);    // Maximum time to start first transmission
        double ttxIdle_min @unit(s) = default(90s);     // Minimum idle time between transmissions
        double ttxIdle_max @unit(s) = default(110s);    // Maximum idle time between transmissions

        int Ntw = default(1);                           // Number of transmission windows
        int Ntx = default(6);                           // Number of transmissions per window

        // Satellite detection parameters
        bool enableSatDetect = default(false);          // Enable satellite detection mechanism
        double tsdStart_min @unit(s) = default(0s);     // Minimum time before first detection period
        double tsdStart_max @unit(s) = default(60s);    // Maximum time before first detection period
        int Ndetect = default(4);                       // Number of detection batches
        int NsdSync = default(2);                       // Number of detection periods per batch
        double tsyncIdle @unit(s) = default(60s);        // Idle time between detection periods in a batch
        double tsdIdle @unit(s) = default(240s);         // Idle time between detection batches
        double satelliteDetectionProbability = default(0.9); // Probability of detecting satellite when in view
        double minimumElevation = default(5.0);         // Minimum elevation angle (degrees) for satellite detection

        // Satellite detection listen timings
        double detectInitialListen @unit(s) = default(0.330s);    // Initial listen duration
        double detectSuccessExtraMin @unit(s) = default(0.075s);  // Min extra listen when detected
        double detectSuccessExtraMax @unit(s) = default(1.670s);  // Max extra listen when detected
        double detectMissExtra @unit(s) = default(1.670s);        // Extra listen when miss in view

        // lora parameters
        int LoRaSF = default(10);
        double LoRaTP = default(25);
        double LoRaCF = default(868000000);
        int LoRaBW = default(125000);
        int LoRaCR = default(1);
		
		bool useMLBox = default(false);
    submodules:
        queue: <default("DropTailQueue")> like IPacketQueue {
            parameters:
                packetCapacity = default(-1);
                @display("p=100,100;q=l2queue");
        }
		mlBox: Transmission_Predictor if useMLBox{
			 @display("p=100,250");
		}
}

package Florasat.Test_Individual;

import Nodes._10_Terminal.Terminal;
import Nodes._20_Satellite.Satellite;
import Nodes._30_Station.Station;
import Nodes._40_Server.Server;
import Global.Constellation_Creator.ConstellationCreator;
import Global.Topology_Control.TopologyControlBase;
import Global.Topology_Control.ConstellationTopologyControl;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import Global.Medium.DSL.LoRaMedium;
import Global.Statistics.TransmissionStatistics.TransmissionStatistics;
import Global.Transmission_Predictor.Transmission_Predictor;

import Nodes._20_Satellite._60_Mobility.Sat_Mob_Orchestrator;

channel BKH extends ned.DatarateChannel
{
    datarate = 1Gbps;
    delay = 0s;
    @display("ls=gray,2");
}

network Florasat_Individual_Sats{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth_backup");
        int numTerminals = default(30);
        int numSatellites = default(16);
        int numStations = default(1);

		int stats_altitude = default(600);
		int stats_raan = default(210);
		
		string train_or_test = default("");
    submodules:
        radioMedium: LoRaMedium {
            @display("p=121,1000");
        }

        terminal[numTerminals]: Terminal {
            @display("p=662,777");
        }

        satellite[numSatellites]: Satellite {
            @display("p=953,363");
        }

        station[numStations]: Station {
            @display("p=1450,777");
        }

        visualizer: IntegratedVisualizer {
            @display("p=121,825");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=121,272;i=block/routing");
        } // network autoconfiguration

        transmissionStatistics: TransmissionStatistics{
             @display("p=121,825");
        }
        transmissionPredictor: Transmission_Predictor{
            @display("p=121,825");
        }

    connections allowunconnected:

}
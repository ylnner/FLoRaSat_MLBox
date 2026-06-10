package Florasat;

import Nodes._10_Terminal.Terminal;
import Nodes._20_Satellite.Satellite;
import Nodes._30_Station.Station;
import Nodes._40_Server.Server;
import Global.Constellation_Creator.ConstellationCreator;
import Global.Topology_Control.TopologyControlBase;
import Global.Topology_Control.ConstellationTopologyControl;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import Global.Medium.DSL.CustomMedium;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadioMedium;

import Nodes._20_Satellite._60_Mobility.Sat_Mob_Orchestrator;

channel BKH extends ned.DatarateChannel
{
    datarate = 1Gbps;
    delay = 0s;
    @display("ls=gray,2");
}

network Florasat
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth_backup");
        int numTerminals = default(30);
        int numSatellites = default(16);
        int numStations = default(1);

    submodules:
        radioMedium: <default("LoRaMedium")> like IRadioMedium{
            @display("p=121,1000");
        }

        terminal[numTerminals]: Terminal {
            @display("p=662,777");
        }

        constellationCreator: ConstellationCreator {
            @display("p=121,417;i=block/cogwheel");
        }

        satellite[numSatellites]: Satellite {
            @display("p=953,363");
        }

        station[numStations]: Station {
            @display("p=1450,777");
        }
        topologyControl: ConstellationTopologyControl {
            @display("p=121,540;i=block/network2");
        }

        visualizer: IntegratedVisualizer {
            @display("p=121,825");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=121,272;i=block/routing");
        } // network autoconfiguration


        satMobilityOrchestrator: Sat_Mob_Orchestrator {
            @display("p=121,672;i=block/rightarrow");
        }

    connections allowunconnected:

}

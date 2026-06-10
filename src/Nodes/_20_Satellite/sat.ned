package Nodes._20_Satellite;

import Global.Mobility.INorad;
import Global.Utilities.LayerForwarder;

import Nodes._20_Satellite.DSL._10_Physical.Sat_Dsl_Phy;
import Nodes._20_Satellite.DSL._20_Data_Link.Sat_Dsl_Dlk;

import Nodes._20_Satellite.ISL._10_Physical.Sat_Isl_Phy;
import Nodes._20_Satellite.ISL._20_Data_Link.Sat_Isl_Dlk;

import Nodes._20_Satellite.GSL._10_Physical.Sat_Gsl_Phy;
import Nodes._20_Satellite.GSL._20_Data_Link.Sat_Gsl_Dlk;

import Nodes._20_Satellite._30_Network.Sat_Net;
import Nodes._20_Satellite._40_Application.Sat_App;
import Nodes._20_Satellite._50_Energy.Sat_Ene;
import Nodes._20_Satellite._60_Mobility.Sat_Mob_SatelliteMobility;

module Satellite extends LayerForwarder
{
    parameters:
        @display("p=695.185,549.115;i=satellit_blue");
        @class(satellite::Sat);
        layerName = default("Satellite");

        string satelliteName = default("");
        string noradModuleType = default("");

        //*.interfaceTableModule = default(absPath(".interfaceTable"));
        //*.net.nic.interfaceTableModule = default(absPath("^.interfaceTable"));
        // *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        // *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");

        double upperLatitudeBound = default(70.0);
        double lowerLatitudeBound = default(-70.0);
        
		// Mobility selection
        string mobilityType = default("Sat_Mob_SatelliteMobility");

        int numGroundLinks = default(60);
    gates:
        // ISL gates
        input leftIn;
        output leftOut;
        input upIn;
        output upOut;
        input rightIn;
        output rightOut;
        input downIn;
        output downOut;

        input groundLinkIn[numGroundLinks];
        output groundLinkOut[numGroundLinks];


    submodules:
        phyDSL: Sat_Dsl_Phy {
            @display("p=100,260");
        }
        dlkDSL: Sat_Dsl_Dlk {
            @display("p=100,190");
        }

        phyISL: Sat_Isl_Phy {
            @display("p=170,260");
        }
        dlkISL: Sat_Isl_Dlk {
            @display("p=170,190");
        }

        phyGSL: Sat_Gsl_Phy {
            @display("p=240,260");
        }
        dlkGSL: Sat_Gsl_Dlk {
            @display("p=240,190");
        }

        net: Sat_Net {
            @display("p=170,120"); //is=n
            numGroundLinks = parent.numGroundLinks;
        }
        app: Sat_App {
            @display("p=170,50");
        }
        energy: Sat_Ene {
            @display("p=355,167");
        }
        //mobility: Sat_Mob_SatelliteMobility {
        //    @display("p=355,89");
        //}        
        mobility: <mobilityType> like Global.Mobility.Base_IMobility {
            @display("p=355,89");
        }
        NoradModule: <noradModuleType> like Global.Mobility.INorad if noradModuleType != "" {
            @display("p=354,241");
        }


    connections allowunconnected:
        dlkDSL.lowerOut --> phyDSL.upperIn;
        phyDSL.upperOut --> dlkDSL.lowerIn;

        dlkISL.lowerOut --> phyISL.upperIn;
        phyISL.upperOut --> dlkISL.lowerIn;

        dlkGSL.lowerOut --> phyGSL.upperIn;
        phyGSL.upperOut --> dlkGSL.lowerIn;

        net.lowerOutDSL --> dlkDSL.upperIn;
        dlkDSL.upperOut --> net.lowerInDSL;

        net.lowerOutISL --> dlkISL.upperIn;
        dlkISL.upperOut --> net.lowerInISL;

        net.lowerOutGSL --> dlkGSL.upperIn;
        dlkGSL.upperOut --> net.lowerInGSL;

        ///app.lowerOut --> net.upperIn;
        ///net.upperOut --> app.lowerIn;
        
        net.leftOut --> leftOut;
        net.upOut --> upOut;
        net.downOut --> downOut;
        net.rightOut --> rightOut;
        
        leftIn --> net.leftIn;
        upIn --> net.upIn;
        downIn --> net.downIn;
        rightIn --> net.rightIn;

        for i=0..numGroundLinks-1 {
            net.groundLinkOut[i] --> groundLinkOut[i];
            groundLinkIn[i] --> net.groundLinkIn[i];        
        }
                
}

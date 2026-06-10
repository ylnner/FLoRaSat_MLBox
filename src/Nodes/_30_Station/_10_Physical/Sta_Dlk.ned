package Nodes._30_Station._10_Physical;

simple Sta_Phy
{
	
    parameters:
        @class(station::Sta_Phy);
        @display("i=block/tunnel");
    gates:
        inout satLink[60] @labels(EtherFrame-conn);
		output upperOut;
}

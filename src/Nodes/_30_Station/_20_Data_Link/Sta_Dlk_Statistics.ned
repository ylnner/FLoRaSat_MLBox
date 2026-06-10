package Nodes._30_Station._20_Data_Link;

simple Sta_Dlk_Statistics
{
    parameters:
        @class(mac::Sta_Dlk_Statistics);
        @display("i=block/mac");
        bool endToEndStatistics = default(true);  // Control end-to-end delay computation
        
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input lowerLayerIn;
        output lowerLayerOut;
}
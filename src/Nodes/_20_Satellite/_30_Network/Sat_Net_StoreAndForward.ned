package Nodes._20_Satellite._30_Network;

import Global.Base._30_Network.Base_Routing;

simple StoreAndForward extends Base_Routing
{
    parameters:
        @class(routing::Sat_Net_StoreAndForward);
        @display("i=block/routing");
        
        int maxStoredPackets = default(1000); // Maximum number of packets that can be stored
}
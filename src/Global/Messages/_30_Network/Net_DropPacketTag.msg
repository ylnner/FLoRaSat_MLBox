import inet.common.TagBase;
import inet.common.Simsignals;

//import core.ISLDirection;

namespace satellite;

class Sat_Net_DropPacketTag extends inet::TagBase
{
    bool silent = false;
    inet::PacketDropReason reason;
}
import inet.common.TagBase;

namespace routing;

enum CstPacketType {
    NORMAL = 0;
    CONTROL = 1;
};

enum HopType {
    GS = 0;
    SAT = 1;
    DEV = 2;
}

struct Hop
{
    HopType type;
    int id;
    double lat;
    double lon;
    uint16_t alt;
}

class CstRoutingTag extends inet::TagBase {
    CstPacketType type;
    int srcGs = -1;
    int dstGs = -1;
    int srcId = -1;
    int dstId = -1;    
    int srcSat = -1;
    int dstSat = -1;
    int hops = 0;
    int maxHops = -1;
    int isLoraNode = 0;
    int srcGsOrDev = 0;
    int dstGsOrDev = 0;
    bool roundTrip = false;
    Hop route[];
}
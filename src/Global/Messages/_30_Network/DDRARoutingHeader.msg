import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;

cplusplus {{
using namespace inet;
}}

namespace routing::ddra;

enum PacketType {
    NORMAL = 0;
    CONGESTED = 1;
    UNCONGESTED = 2;
    FAILURE = 3;
};

class DDRARoutingHeader extends inet::FieldsChunk {
    PacketType type;
    int srcSat = -1;
    int dstSat = 1;
    int srcGs = -1;
    int dstGs = -1;
    int failureTarget = -1;
}
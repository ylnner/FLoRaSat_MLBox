import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;
import Global.Messages._20_Data_Link.Base_MacFrame;
//import Base_MacFrame;

cplusplus {{
using namespace inet;
}}

namespace mac;

// Frame type definitions 
enum KiWanFrameType {
    UL_DATA = 0;      // Uplink data frame
};

class KiWanFrame extends Base_MacFrame {    
    int kind @enum(KiWanFrameType);
    int id; // Frame identifier
    int srcId; // Source identifier
    int repetition; // Repetition count

    // Statistics collection
    double RSSI;
    double SNIR;

    // Use to compute delay
    double queuedAt;
    double sentAt;
}
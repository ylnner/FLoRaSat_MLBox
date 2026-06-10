import inet.common.TagBase;

namespace satellite;

class Sat_Net_QueueInsertionTimeTag extends inet::TagBase
{
    simtime_t insertionTime = inet::simTime();
}

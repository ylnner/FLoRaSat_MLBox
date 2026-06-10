import inet.common.TagBase;

namespace satellite;

class SendAtTag extends inet::TagBase
{
    simtime_t sendAt = inet::simTime();
}

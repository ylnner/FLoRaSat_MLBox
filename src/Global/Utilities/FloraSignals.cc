#include "FloraSignals.h"

namespace core {

omnetpp::simsignal_t controlPacketSentSignal = omnetpp::cComponent::registerSignal("controlPacketSent");
omnetpp::simsignal_t controlPacketReceivedSignal = omnetpp::cComponent::registerSignal("controlPacketReceived");

}  // namespace core

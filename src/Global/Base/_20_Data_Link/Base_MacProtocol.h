#ifndef FLORA_BASE_MACPROTOCOL_H_
#define FLORA_BASE_MACPROTOCOL_H_

#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/queueing/contract/IPacketQueue.h"

using namespace omnetpp;

namespace mac {

/**
 * @brief Base class for MAC protocol implementations without INET MacProtocolBase dependency
 * 
 * This class provides essential MAC layer functionality similar to INET's MacProtocolBase
 * but is independent of INET's lifecycle and operational framework, making it suitable
 * for custom implementations in the FLoRaSat simulator.
 */
class Base_MacProtocol : public cSimpleModule, public cListener {
public:
        virtual inet::MacAddress getAddress() = 0;
protected:
    /** @brief Gate IDs for communication between layers */
    int upperLayerInGateId = -1;
    int upperLayerOutGateId = -1;
    int lowerLayerInGateId = -1;
    int lowerLayerOutGateId = -1;

    /** @brief Network interface reference */
    inet::NetworkInterface *networkInterface = nullptr;

    /** @brief Currently transmitted frame if any */
    inet::Packet *currentTxFrame = nullptr;

    /** @brief Transmission queue for messages from upper layer */
    inet::queueing::IPacketQueue *txQueue = nullptr;

    /** @brief Host module reference */
    cModule *hostModule = nullptr;

    /** @brief MAC address of this interface */
    inet::MacAddress macAddress;

    /** @brief Operational state */
    enum State {
        STATE_DOWN,
        STATE_UP
    };
    State operationalState = STATE_DOWN;

protected:
    Base_MacProtocol();
    virtual ~Base_MacProtocol();

    // Initialization and lifecycle
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void finish() override;

    // Message handling
    virtual void handleMessage(cMessage *message) override;
    virtual void handleMessageWhenUp(cMessage *message);
    virtual void handleMessageWhenDown(cMessage *message);

    virtual void handleSelfMessage(cMessage *message);
    virtual void handleUpperMessage(cMessage *message);
    virtual void handleLowerMessage(cMessage *message);

    virtual void handleUpperPacket(inet::Packet *packet);
    virtual void handleLowerPacket(inet::Packet *packet);

    // Message routing
    virtual bool isUpperMessage(cMessage *message);
    virtual bool isLowerMessage(cMessage *message);

    // Send functions
    virtual void sendUp(cMessage *message);
    virtual void sendDown(cMessage *message);

    // MAC address utilities
    virtual inet::MacAddress parseMacAddressParameter(const char *addrstr);

    // Frame management
    virtual void deleteCurrentTxFrame();
    virtual void dropCurrentTxFrame(inet::PacketDropDetails& details);
    virtual void popTxQueue();
    virtual void flushQueue(inet::PacketDropDetails& details);
    virtual void clearQueue();

    // State management
    virtual void startOperation();
    virtual void stopOperation();
    virtual void crashOperation();

    // Signal handling
    using cListener::receiveSignal;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    // Helper for time-on-air calculation (from Base_Mac)
    double computeTimeOnAir(int packetLenBits, int SF, int BW, int CR);
};

} // namespace mac

#endif  // FLORA_BASE_MACPROTOCOL_H_

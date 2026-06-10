#include "Base_MacProtocol.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
#include <cmath>

namespace mac {

Base_MacProtocol::Base_MacProtocol()
{
}

Base_MacProtocol::~Base_MacProtocol()
{
    delete currentTxFrame;
    if (hostModule)
        hostModule->unsubscribe(inet::interfaceDeletedSignal, this);
}

void Base_MacProtocol::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // Initialize gate IDs
        currentTxFrame = nullptr;
        upperLayerInGateId = findGate("upperLayerIn");
        upperLayerOutGateId = findGate("upperLayerOut");
        lowerLayerInGateId = findGate("lowerLayerIn");
        lowerLayerOutGateId = findGate("lowerLayerOut");
        
        // Get host module reference
        hostModule = inet::findContainingNode(this);
        if (hostModule)
            hostModule->subscribe(inet::interfaceDeletedSignal, this);
            
        // Initialize operational state
        operationalState = STATE_DOWN;
    }
    else if (stage == inet::INITSTAGE_LINK_LAYER) {
        // Start operation at link layer stage
        startOperation();
    }
}

void Base_MacProtocol::finish()
{
    // Override in derived classes for statistics recording
}

void Base_MacProtocol::handleMessage(cMessage *message)
{
    if (operationalState == STATE_UP)
        handleMessageWhenUp(message);
    else
        handleMessageWhenDown(message);
}

void Base_MacProtocol::handleMessageWhenUp(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else if (isUpperMessage(message))
        handleUpperMessage(message);
    else if (isLowerMessage(message))
        handleLowerMessage(message);
    else
        throw cRuntimeError("Message '%s' received on unexpected gate '%s'", 
                          message->getName(), message->getArrivalGate()->getFullName());
}

void Base_MacProtocol::handleMessageWhenDown(cMessage *message)
{
    if (!message->isSelfMessage() && message->getArrivalGateId() == lowerLayerInGateId) {
        EV << "Interface is turned off, dropping packet\n";
        delete message;
    }
    else {
        // Handle self messages or delete others
        if (message->isSelfMessage())
            handleSelfMessage(message);
        else
            delete message;
    }
}

void Base_MacProtocol::handleSelfMessage(cMessage *message)
{
    throw cRuntimeError("Self message '%s' is not handled.", message->getName());
}

void Base_MacProtocol::handleUpperMessage(cMessage *message)
{
    if (!message->isPacket())
        delete message;  // Ignore commands by default
    else {
        emit(inet::packetReceivedFromUpperSignal, message);
        handleUpperPacket(check_and_cast<inet::Packet *>(message));
    }
}

void Base_MacProtocol::handleLowerMessage(cMessage *message)
{
    if (!message->isPacket())
        delete message;  // Ignore commands by default
    else {
        emit(inet::packetReceivedFromLowerSignal, message);
        handleLowerPacket(check_and_cast<inet::Packet *>(message));
    }
}

void Base_MacProtocol::handleUpperPacket(inet::Packet *packet)
{
    throw cRuntimeError("Upper packet '%s' is not handled.", packet->getName());
}

void Base_MacProtocol::handleLowerPacket(inet::Packet *packet)
{
    throw cRuntimeError("Lower packet '%s' is not handled.", packet->getName());
}

bool Base_MacProtocol::isUpperMessage(cMessage *message)
{
    return message->getArrivalGateId() == upperLayerInGateId;
}

bool Base_MacProtocol::isLowerMessage(cMessage *message)
{
    return message->getArrivalGateId() == lowerLayerInGateId;
}

void Base_MacProtocol::sendUp(cMessage *message)
{
    if (message->isPacket())
        emit(inet::packetSentToUpperSignal, message);
    send(message, upperLayerOutGateId);
}

void Base_MacProtocol::sendDown(cMessage *message)
{
    if (message->isPacket())
        emit(inet::packetSentToLowerSignal, message);
    send(message, lowerLayerOutGateId);
}

inet::MacAddress Base_MacProtocol::parseMacAddressParameter(const char *addrstr)
{
    inet::MacAddress address;

    if (!strcmp(addrstr, "auto"))
        // assign automatic address
        address = inet::MacAddress::generateAutoAddress();
    else
        address.setAddress(addrstr);

    return address;
}

void Base_MacProtocol::deleteCurrentTxFrame()
{
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void Base_MacProtocol::dropCurrentTxFrame(inet::PacketDropDetails& details)
{
    emit(inet::packetDroppedSignal, currentTxFrame, &details);
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void Base_MacProtocol::popTxQueue()
{
    if (currentTxFrame != nullptr)
        throw cRuntimeError("Model error: incomplete transmission exists");
    ASSERT(txQueue != nullptr);
    currentTxFrame = txQueue->dequeuePacket();
    currentTxFrame->setArrival(getId(), upperLayerInGateId, simTime());
    take(currentTxFrame);
}

void Base_MacProtocol::flushQueue(inet::PacketDropDetails& details)
{
    if (txQueue) {
        while (!txQueue->isEmpty()) {
            auto packet = txQueue->dequeuePacket();
            emit(inet::packetDroppedSignal, packet, &details);
            delete packet;
        }
    }
}

void Base_MacProtocol::clearQueue()
{
    if (txQueue) {
        while (!txQueue->isEmpty())
            delete txQueue->dequeuePacket();
    }
}

void Base_MacProtocol::startOperation()
{
    operationalState = STATE_UP;
    if (networkInterface) {
        networkInterface->setState(inet::NetworkInterface::State::UP);
        networkInterface->setCarrier(true);
    }
}

void Base_MacProtocol::stopOperation()
{
    inet::PacketDropDetails details;
    details.setReason(inet::INTERFACE_DOWN);
    if (currentTxFrame)
        dropCurrentTxFrame(details);
    flushQueue(details);
    
    if (networkInterface) {
        networkInterface->setCarrier(false);
        networkInterface->setState(inet::NetworkInterface::State::DOWN);
    }
    operationalState = STATE_DOWN;
}

void Base_MacProtocol::crashOperation()
{
    deleteCurrentTxFrame();
    clearQueue();
    
    if (networkInterface) {
        networkInterface->setCarrier(false);
        networkInterface->setState(inet::NetworkInterface::State::DOWN);
    }
    operationalState = STATE_DOWN;
}

void Base_MacProtocol::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method("receiveSignal");
    if (signalID == inet::interfaceDeletedSignal) {
        if (networkInterface == check_and_cast<const inet::NetworkInterface *>(obj))
            networkInterface = nullptr;
    }
}

double Base_MacProtocol::computeTimeOnAir(int packetLenBits, int SF, int BW, int CR)
{
    const int nPreamble = 8;
    const double payloadBytes = static_cast<double>(packetLenBits) / 8.0;

    int payloadSymbNb = 8;
    payloadSymbNb += std::ceil((8 * payloadBytes - 4 * SF + 28 + 16 - 20 * 0) / (4 * (SF - 2 * 0))) * (CR + 4);
    if (payloadSymbNb < 8)
        payloadSymbNb = 8;

    const double Tsym = std::pow(2, SF) / BW;
    const double Tpreamble = (nPreamble + 4.25) * Tsym;
    const double Theader = 0.5 * (8 + payloadSymbNb) * Tsym;
    const double Tpayload = 0.5 * (8 + payloadSymbNb) * Tsym;

    return Tpreamble + Theader + Tpayload;
}

}  // namespace mac

#include "Sat_Dsl_Dlk_kMac.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"

#include "Global/Messages/_10_Physical/LoRaPhyPreamble_m.h"
#include "Global/Utilities/libnorad/cEcef.h"
#include "Global/Utilities/Utils.h"
#include "Nodes/_10_Terminal/_40_Application/Ter_App.h"
#include "Nodes/_20_Satellite/DSL/_10_Physical/Sat_Dsl_Phy_LoRaRadio.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"

#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cmath>

namespace mac {

// Register the module with OMNeT++
Define_Module(Sat_Dsl_Dlk_kMac);

// Initialize shared memory between satellite instances for statistics 
std::unordered_map<int, bool> Sat_Dsl_Dlk_kMac::sharedUniqueFrames;
std::unordered_map<std::string, bool> Sat_Dsl_Dlk_kMac::sharedDuplicateFrames;


Sat_Dsl_Dlk_kMac::~Sat_Dsl_Dlk_kMac() {
    cancelAndDelete(eventStartListening);
}

void Sat_Dsl_Dlk_kMac::initialize(int stage) {
    Base_MacProtocol::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        getSimulation()->getSystemModule()->subscribe("LoRaGWRadioReceptionFinishedCorrect", this);
        getSimulation()->getSystemModule()->subscribe("LoRaGWRadioReceptionFinishedIgnoring", this);
        cModule *phy = getParentModule()->getParentModule()->getSubmodule("phyDSL");
        if (!phy)
            throw cRuntimeError("PHY module not found! Expected submodule named 'phyDSL'");

        // Get the radio inside PHY
        cModule *radioModule = phy->getSubmodule("radio");
        if (!radioModule)
            throw cRuntimeError("Radio submodule not found inside PHY!");

        radio = check_and_cast<IRadio *>(radioModule);
        
        // Initialize LoRa parameters
        LoRaSF = par("LoRaSF");
        LoRaTP = par("LoRaTP");
        LoRaCF = par("LoRaCF");
        LoRaBW = par("LoRaBW");
        LoRaCR = par("LoRaCR");
        
        // Initialize fixed loss rate parameter (default to 0 if not specified)
        fixedLossRate = par("fixedLossRate").doubleValue();
        
        debugOutput = true;

        const char *addressString = par("address");
        if (!strcmp(addressString, "auto")) {
            address = MacAddress::generateAutoAddress(); // assign automatic address
            par("address").setStringValue(address.str().c_str()); // change module parameter from "auto" to concrete address
        }
        else {
            address.setAddress(addressString);
        }

        // Initialize vector recorders for delay statistics
        allSendingDelayVector = new cOutVector("All Nodes Message Sending Delay at Satellite");
        allGenerationDelayVector = new cOutVector("All Nodes Message Generation Delay at Satellite");
        
        // Initialize vector recorders for repetition delay statistics
        allRepSendingDelayVector = new cOutVector("All Nodes Repetition Sending Delay at Satellite");
        allRepGenerationDelayVector = new cOutVector("All Nodes Repetition Generation Delay at Satellite");

        // Initialize position vector recorders
        repetitionPositionVector = new cOutVector("Satellite Position when Repetition Received");
        duplicationPositionVector = new cOutVector("Satellite Position when Duplication Received");

        scheduleAt(simTime(), this->eventStartListening);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(inet::physicallayer::IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void Sat_Dsl_Dlk_kMac::finish() {
    
    sendingDelayVectors.clear();
    generationDelayVectors.clear();
    repSendingDelayVectors.clear();
    repGenerationDelayVectors.clear();
    repPositionVectors.clear();
    dupPositionVectors.clear();
    
    // Record the total duplicates counter as a scalar
    recordScalar("Number of duplicate transmissions", numDuplicates);
    
    // Record per-node duplication counts as scalars
    for (const auto& pair : duplicationsPerNode) {
        std::string scalarName = "Number of duplications for Node " + std::to_string(pair.first);
        recordScalar(scalarName.c_str(), pair.second);
    }
    
    // Record the number of dropped packets due to fixed loss
    recordScalar("Number of packets dropped due to fixed loss", numDroppedPackets);
    recordScalar("Number of successful receptions", numReceptions);


}

void Sat_Dsl_Dlk_kMac::handleSelfMessage(cMessage *msg) {
    switch (currentState) {
        case KIWAN_LISTEN:
            handleListenState(msg);
            break;
        case KIWAN_RESPOND:
            handleResponseState(msg);
            break;
    }
}

void Sat_Dsl_Dlk_kMac::handleListenState(cMessage *msg) {
    // Just set the radio to receiver mode once
    if (msg == this->eventStartListening) {
        radio->setRadioMode(inet::physicallayer::IRadio::RADIO_MODE_RECEIVER);
        // No need to reschedule - the MAC will now passively listen
        EV_INFO << "Sat_Dsl_Dlk_kMac switched to LISTEN state" << endl;
    }
}


void Sat_Dsl_Dlk_kMac::handleResponseState(cMessage *msg) {
    currentState = KIWAN_LISTEN;
}


void Sat_Dsl_Dlk_kMac::handleUpperMessage(cMessage *msg) {
}

void Sat_Dsl_Dlk_kMac::handleLowerMessage(cMessage *msg) {

    auto packet = check_and_cast<Packet *>(msg);

    auto frontChunk = packet->peekAtFront<inet::Chunk>();
    if (dynamicPtrCast<const messages::LoRaPhyPreamble>(frontChunk) != nullptr) {
        // It found LoRaPhyPreamble and remove it
        packet->popAtFront<messages::LoRaPhyPreamble>();
    }

    //packet->popAtFront<messages::LoRaPhyPreamble>();
    const auto& frame = packet->peekAtFront<KiWanFrame>();
    
    // Apply fixed packet loss - randomly drop packets based on configured loss rate
    if (fixedLossRate > 0 && uniform(0, 1) < fixedLossRate) {
        EV_INFO << "Dropping packet due to simulated fixed loss" << endl;
        numDroppedPackets++;
        delete msg;  // Drop the packet
        return;
    }
    
    if (frame->getKind() == UL_DATA) {
        if (hasGUI()) getParentModule()->getParentModule()->bubble("RECEIVED DATA FRAME");
        
        // Get satellite position for recording using the working method
        double satLatitude = 0.0;
        double satLongitude = 0.0;
        bool positionFound = false;

        // Get position through radio's mobility interface
        if (radio) {
            try {
                auto antenna = radio->getAntenna();
                if (antenna) {
                    auto mobility = antenna->getMobility();
                    if (mobility) {
                        auto satMob = dynamic_cast<mobility::Sat_Mob_SatelliteMobility*>(mobility);
                        if (satMob) {
                            satLatitude = satMob->getLatitude();
                            satLongitude = satMob->getLongitude();
                            if (satLatitude != 0.0 || satLongitude != 0.0) {
                                positionFound = true;
                                EV_INFO << "Position found via Radio->SatMobility: (" << satLatitude << ", " << satLongitude << ")" << endl;
                            }
                        }
                    }
                }
            } catch (...) {
                EV_INFO << "Failed to get position from Radio mobility interface" << endl;
            }
        }

        if (!positionFound) {
            EV_WARN << "Could not retrieve satellite position - using default (0,0)" << endl;
        }
        
        // for ML Box metric
        this->numReceptions++;

        // Track per-node statistics
        int nodeId = frame->getSrcId();
            
        // Calculate and record packet delay
        double packetDelayFromGeneration = simTime().dbl() - frame->getQueuedAt();
        double packetDelayFromSending = simTime().dbl() - frame->getSentAt();
        
        // Track unique frames and record vector statistics
        int frameId = frame->getId();
        
        // Create a unique key for this specific transmission using frameId and sentAt timestamp
        simtime_t sentTime = frame->getSentAt();
        std::string transmissionKey = std::to_string(frameId) + "_" + std::to_string(sentTime.dbl());
        
        // Check if this is a duplication (same transmission received by multiple satellites)
        bool isDuplication = (sharedDuplicateFrames.find(transmissionKey) != sharedDuplicateFrames.end());
        
        if (isDuplication) {
            // This is a duplication (same transmission received by multiple satellites)
            EV_INFO << "Received duplicate transmission with key: " << transmissionKey 
                    << ", source node: " << nodeId
                    << ", sending delay: " << packetDelayFromSending
                    << ", generation delay: " << packetDelayFromGeneration
                    << ", satellite position: (" << satLatitude << ", " << satLongitude << ")" << endl;
                    
            // Record satellite position when duplication is received
            // Round to nearest integer and encode position: latitude * 1000 + (longitude + 180)
            int roundedLatitude = (int)round(satLatitude);
            int roundedLongitude = (int)round(satLongitude);
            double positionEncoded = roundedLatitude * 1000.0 + (roundedLongitude + 180.0);
            duplicationPositionVector->record(positionEncoded);
            
            // Create per-node duplication position vector if not exists
            if (dupPositionVectors.find(nodeId) == dupPositionVectors.end()) {
                std::string vectorName = "Duplication Position for Node " + std::to_string(nodeId) + " at Satellite";
                dupPositionVectors[nodeId] = new cOutVector(vectorName.c_str());
            }
            dupPositionVectors[nodeId]->record(positionEncoded);
                    
            // Increment the duplicates counter (global and per-node)
            numDuplicates++;
            duplicationsPerNode[nodeId]++;
        } else {
            // This is the first reception of this specific transmission
            sharedDuplicateFrames[transmissionKey] = true;
            
            // Create repetition vectors if not exists
            if (repSendingDelayVectors.find(nodeId) == repSendingDelayVectors.end()) {
                std::string vectorName = "Repetition Sending Delay for Node " + std::to_string(nodeId) + " at Satellite";
                repSendingDelayVectors[nodeId] = new cOutVector(vectorName.c_str());
                
                std::string genVectorName = "Repetition Generation Delay for Node " + std::to_string(nodeId) + " at Satellite";
                repGenerationDelayVectors[nodeId] = new cOutVector(genVectorName.c_str());
            }
            
            // Record satellite position when repetition is received
            // Round to nearest integer and encode position: latitude * 1000 + (longitude + 180)
            int roundedLatitude = (int)round(satLatitude);
            int roundedLongitude = (int)round(satLongitude);
            double positionEncoded = roundedLatitude * 1000.0 + (roundedLongitude + 180.0);
            repetitionPositionVector->record(positionEncoded);
            
            // Create per-node repetition position vector if not exists
            if (repPositionVectors.find(nodeId) == repPositionVectors.end()) {
                std::string vectorName = "Repetition Position for Node " + std::to_string(nodeId) + " at Satellite";
                repPositionVectors[nodeId] = new cOutVector(vectorName.c_str());
            }
            repPositionVectors[nodeId]->record(positionEncoded);
                
            // allRepSendingDelayVector->record(packetDelayFromSending);
            allRepGenerationDelayVector->record(packetDelayFromGeneration);
            // repSendingDelayVectors[nodeId]->record(packetDelayFromSending);
            repGenerationDelayVectors[nodeId]->record(packetDelayFromGeneration);
            
            // Check if this is a new unique frame ID (for tracking purposes only)
            if (sharedUniqueFrames.find(frameId) == sharedUniqueFrames.end()) {
                // This is a new unique frame ID, add it to the set
                sharedUniqueFrames[frameId] = true;
                
                EV_INFO << "Received unique frame with ID: " << frameId 
                        << ", source node: " << nodeId
                        << ", sending delay: " << packetDelayFromSending
                        << ", generation delay: " << packetDelayFromGeneration
                        << ", satellite position: (" << satLatitude << ", " << satLongitude << ")" << endl;
                
                // Create unique frame vectors if not exists
                if (sendingDelayVectors.find(nodeId) == sendingDelayVectors.end()) {
                    std::string vectorName = "Message Sending Delay for Node " + std::to_string(nodeId) + " at Satellite";
                    sendingDelayVectors[nodeId] = new cOutVector(vectorName.c_str());
                    
                    std::string genVectorName = "Message Generation Delay for Node " + std::to_string(nodeId) + " at Satellite";
                    generationDelayVectors[nodeId] = new cOutVector(genVectorName.c_str());
                }

                // Record delay for unique frames only
                // allSendingDelayVector->record(packetDelayFromSending);
                allGenerationDelayVector->record(packetDelayFromGeneration);
                // sendingDelayVectors[nodeId]->record(packetDelayFromSending);
                generationDelayVectors[nodeId]->record(packetDelayFromGeneration);
            } else {
                EV_INFO << "Received repetition of frame with ID: " << frameId 
                        << ", source node: " << nodeId
                        << ", sending delay: " << packetDelayFromSending
                        << ", generation delay: " << packetDelayFromGeneration
                        << ", satellite position: (" << satLatitude << ", " << satLongitude << ")" << endl;
            }
        }
    }
    EV_INFO << "Send packet to net layer" << endl;
    EV_INFO << packet->getDetailStringRepresentation(evFlags) << endl;
    sendUp(packet);
}

MacAddress Sat_Dsl_Dlk_kMac::getAddress() {
    return address;
}

void Sat_Dsl_Dlk_kMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) {
    Enter_Method_Silent();
    const char *signalName = getSignalName(signalID);
    if (strcmp(signalName, "LoRaGWRadioReceptionFinishedIgnoring") == 0 && (int) value == par("satIndex").intValue()) {
        numCollisions++;    
    }
}
} // namespace mac

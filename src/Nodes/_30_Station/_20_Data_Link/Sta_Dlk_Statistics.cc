#include "inet/common/packet/Packet.h"
#include "inet/common/InitStages.h"

#include "Sta_Dlk_Statistics.h"
#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"

namespace mac {

// Initialize static member variables
std::unordered_map<int, bool> Sta_Dlk_Statistics::sharedUniqueFrames;
std::unordered_map<int, int> Sta_Dlk_Statistics::sharedRepetitionCounts;

Define_Module(Sta_Dlk_Statistics);

Sta_Dlk_Statistics::~Sta_Dlk_Statistics() {
    // Clean up vector recorders
    delete allSendingDelayVector;
    delete allGenerationDelayVector;
    
    // Clean up per-sender vectors
    for (auto& pair : sendingDelayVectors) {
        delete pair.second;
    }
    for (auto& pair : generationDelayVectors) {
        delete pair.second;
    }
}

void Sta_Dlk_Statistics::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        // MAC-specific initialization
        gsId = getParentModule()->getParentModule()->getIndex();
        totalPacketsReceived = 0;
        packetsReceivedDuplicate = 0;
        
        // Read configuration parameter
        endToEndStatistics = par("endToEndStatistics").boolValue();
        
        // Initialize vector recorders only if end-to-end statistics are enabled
        if (endToEndStatistics) {
            allSendingDelayVector = new cOutVector("All Nodes Sending Delay");
            allGenerationDelayVector = new cOutVector("All Nodes Generation Delay");
        }
        
        // Per-node vectors will be created dynamically when frames arrive
        }
        
        EV_INFO << "Sta_Dlk_Statistics initialized at GS " << gsId 
                << " (end-to-end statistics: " << (endToEndStatistics ? "enabled" : "disabled") << ")" << endl;
    }

void Sta_Dlk_Statistics::handleMessage(cMessage *msg) {
    // Check if this is a packet we should handle
    inet::Packet *pkt = dynamic_cast<inet::Packet *>(msg);
    if (!pkt) {
        EV_WARN << "Sta_Dlk_Statistics received non-packet message, deleting" << endl;
        delete msg;
        return;
    }
    
    EV_INFO << "GS " << gsId << " received MAC frame: " << pkt->getName()
            << ", size: " << pkt->getByteLength() << " bytes" << endl;
    
    // Increment total packets counter
    totalPacketsReceived++;
    
    // Make a copy of the packet to extract KiWanFrame
    inet::Packet *pktCopy = pkt->dup();
    bool foundFrame = false;
    const KiWanFrame* frame = nullptr;
    
    // Try to find KiWanFrame in the packet
    // First check if it's directly at the front
    if (pktCopy->hasAtFront<KiWanFrame>()) {
        frame = pktCopy->peekAtFront<KiWanFrame>().get();
        foundFrame = true;
    } else {
        // If not at front, try to search for it by removing headers one by one
        
        EV_INFO << "  KiWanFrame not at front, trying to find it..." << endl;
        
        while (pktCopy->getTotalLength() > b(0) && !foundFrame) {
            // Try to identify header type before popping
            auto chunk = pktCopy->peekAtFront<inet::Chunk>();
            EV_INFO << "  Found chunk of type: " << chunk->getClassName() << endl;
            
            // Pop the front header
            auto header = pktCopy->popAtFront();
            EV_INFO << "  Removed header: " << header->getClassName() << endl;
            
            // Check if KiWanFrame is now at front
            if (pktCopy->hasAtFront<KiWanFrame>()) {
                frame = pktCopy->peekAtFront<KiWanFrame>().get();
                foundFrame = true;
                EV_INFO << "  Found KiWanFrame after removing headers" << endl;
            }
        }
    }
    
    if (foundFrame) {
        // Only process frame statistics if end-to-end statistics are enabled
        if (endToEndStatistics) {
            // Extract the frame ID and check if it's unique
            int frameId = frame->getId();
            EV_DETAIL << "Processing frame ID: " << frameId << endl;

            if (sharedUniqueFrames.find(frameId) == sharedUniqueFrames.end()) {
                // This is a new unique frame, add it to the set
                sharedUniqueFrames[frameId] = true;
                
                // Get the repetition number for this frame
                int repetition = frame->getRepetition();
                
                // Increment the counter for this repetition number
                sharedRepetitionCounts[repetition]++;
                
                EV_INFO << "  Frame repetition number: " << repetition << endl;
                
                simtime_t currentTime = simTime();
                simtime_t sentTime = frame->getSentAt();
                simtime_t generatedTime = frame->getQueuedAt();

                double sendingDelay = (currentTime - sentTime).dbl();
                double generationDelay = (currentTime - generatedTime).dbl();
                
                EV_INFO << "  Frame sent time: " << sentTime << ", generated time: " << generatedTime 
                          << ", current time: " << currentTime 
                          << ", sending delay: " << sendingDelay 
                          << ", generation delay: " << generationDelay << " seconds" << endl;
                
                // Get the source node ID from the frame
                int senderId = frame->getSrcId();
                EV_INFO << "  Frame source node ID: " << senderId << endl;
                
                // Record to all-senders vectors
                allSendingDelayVector->record(sendingDelay);
                allGenerationDelayVector->record(generationDelay);
                
                // Create sender-specific vectors if they don't exist yet
                if (sendingDelayVectors.find(senderId) == sendingDelayVectors.end()) {
                    std::string vectorName = "Sending Delay for Node " + std::to_string(senderId);
                    sendingDelayVectors[senderId] = new cOutVector(vectorName.c_str());
                    
                    std::string genVectorName = "Generation Delay for Node " + std::to_string(senderId);
                    generationDelayVectors[senderId] = new cOutVector(genVectorName.c_str());
                }
                
                // Record to sender-specific vectors
                sendingDelayVectors[senderId]->record(sendingDelay);
                generationDelayVectors[senderId]->record(generationDelay);
                
                EV_INFO << "  Received unique frame with ID: " << frameId << endl;
            } else {
                packetsReceivedDuplicate++;
                EV_INFO << "  Received duplicate frame with ID: " << frameId << endl;
            }
        }
    } else {
        EV_WARN << "  Could not find KiWanFrame in packet!" << endl;
    }
    
    // Clean up - packet is consumed at GS (endpoint, no forwarding)
    delete pktCopy;
    delete pkt;
}

void Sta_Dlk_Statistics::finish() {    
    // Record MAC layer statistics
    recordScalar("TotalPacketsReceived", totalPacketsReceived);
    
    // Record end-to-end statistics only if enabled
    if (endToEndStatistics) {
        recordScalar("DuplicatePacketsReceived", packetsReceivedDuplicate);
        recordScalar("UniqueFramesReceived", (int)sharedUniqueFrames.size());

        // Record the count for each repetition number
        for (const auto& pair : sharedRepetitionCounts) {
            int repetition = pair.first;
            int count = pair.second;
            std::string scalarName = "messagesReceivedAtRepetition" + std::to_string(repetition);
            recordScalar(scalarName.c_str(), count);
        }
    }
    
    // Note: Vector cleanup is handled in destructor
    EV_INFO << "Sta_Dlk_Statistics statistics recorded for GS " << gsId << endl;
}
}

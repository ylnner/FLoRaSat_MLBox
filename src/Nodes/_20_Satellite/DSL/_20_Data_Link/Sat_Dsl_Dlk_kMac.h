#ifndef TER_DSL_DLK_KMAC_H
#define TER_DSL_DLK_KMAC_H

#include "inet/common/INETDefs.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ModuleAccess.h"

#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"
#include "Global/Base/_20_Data_Link/Base_MacProtocol.h"

#include <map>
#include <unordered_map>

namespace mac {

class Sat_Dsl_Dlk_kMac : public Base_MacProtocol {
    public:
        virtual ~Sat_Dsl_Dlk_kMac();
        
        virtual void initialize(int stage) override;
        virtual void finish() override;

        virtual void handleSelfMessage(cMessage *msg) override;
        virtual void handleUpperMessage(cMessage *msg) override;
        virtual void handleLowerMessage(cMessage *msg) override;
    

        MacAddress getAddress() override;
        
        static std::unordered_map<int, bool> sharedUniqueFrames; // For tracking unique frames
        static std::unordered_map<std::string, bool> sharedDuplicateFrames; // For tracking repetitions

    protected:
        // Basic state
        enum KiWanMacState {
            KIWAN_LISTEN,      // Default listening state
            KIWAN_RESPOND      // Sending response
        };
        KiWanMacState currentState = KIWAN_LISTEN;

        // FSM Events
        cMessage *eventStartListening = new cMessage("Start_Listen");

        MacAddress address;
        
        // LoRa parameters
        int LoRaSF;
        int LoRaCR;
        double LoRaTP;
        double LoRaCF;
        int LoRaBW;

        // Fixed loss rate (0.0 to 1.0)
        double fixedLossRate;                           // Probability of packet loss

        inet::physicallayer::IRadio *radio = nullptr;
        int outGate = -1;

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;

    private:
        // Statistics
        int numCollisions = 0;                       // Count of detected collisions
        int numDuplicates = 0;                       // Count of duplicate transmissions
        int numDroppedPackets = 0;                   // Count of packets dropped due to fixed loss
        std::unordered_map<int, int> packetsPerNode; // Counts packets received from each node
        std::unordered_map<int, int> duplicationsPerNode; // Counts duplications received per node
        
        // Vector recorders for messages
        std::map<int, cOutVector*> sendingDelayVectors;    // Maps node ID to its vector
        std::map<int, cOutVector*> generationDelayVectors; // Maps node ID to its vector
        cOutVector* allSendingDelayVector = nullptr;       // Combined vector for all nodes
        cOutVector* allGenerationDelayVector = nullptr;    // Combined vector for all nodes
        
        // Vector recorders for repetitions
        std::map<int, cOutVector*> repSendingDelayVectors;    // Maps node ID to its repetition sending delay vector
        std::map<int, cOutVector*> repGenerationDelayVectors; // Maps node ID to its repetition generation delay vector
        cOutVector* allRepSendingDelayVector = nullptr;       // Combined vector for all nodes' repetitions
        cOutVector* allRepGenerationDelayVector = nullptr;    // Combined vector for all nodes' repetitions

        // Position vector recorders (encoded data: latitude * 1000 + (longitude + 180))
        cOutVector* repetitionPositionVector = nullptr;   // Position when repetition received (encoded lat:lon)
        cOutVector* duplicationPositionVector = nullptr;  // Position when duplication received (encoded lat:lon)
        std::map<int, cOutVector*> repPositionVectors;    // Per-node position when repetition received
        std::map<int, cOutVector*> dupPositionVectors;    // Per-node position when duplication received

        bool debugOutput;                            // Debug output flag

        // Core handlers following KiWAN protocol specification
        virtual void handleListenState(cMessage *msg);
        virtual void handleResponseState(cMessage *msg);

        int numReceptions = 0;

};

} // namespace mac

#endif

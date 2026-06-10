#ifndef KIWANGSMAC_H_
#define KIWANGSMAC_H_

#include <omnetpp.h>
#include <unordered_map>
#include <map>
#include "inet/common/packet/Packet.h"

using namespace omnetpp;

namespace mac {

class Sta_Dlk_Statistics : public cSimpleModule {
public:
    virtual ~Sta_Dlk_Statistics();
    
protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

private:
    // Ground station identification
    int gsId;
    
    // Configuration parameters
    bool endToEndStatistics;  // Control end-to-end delay computation
    
    // Statistics counters
    int totalPacketsReceived = 0;
    int packetsNotFoundKiWanFrame = 0;
    int packetsReceivedDuplicate = 0;
    
    // Static tracking across all GS instances
    static std::unordered_map<int, bool> sharedUniqueFrames; // For tracking unique frames
    static std::unordered_map<int, int> sharedRepetitionCounts; // Track count of frames by repetition number

    // Vector recorders for delays
    std::map<int, cOutVector*> sendingDelayVectors;    // Maps node ID to its vector
    std::map<int, cOutVector*> generationDelayVectors; // Maps node ID to its vector
    cOutVector* allSendingDelayVector = nullptr;       // Combined vector for all nodes
    cOutVector* allGenerationDelayVector = nullptr;    // Combined vector for all nodes
};

}

#endif

#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/INETDefs.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/Units.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadioMedium.h"
#include "inet/linklayer/common/MacAddress_m.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/PhysicalLayerDefs.h"

#include <iostream>
#include <cmath>

#include "Ter_Dlk_KiWan.h"
#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"
#include "Global/Messages/_30_Network/Net_CstRoutingTag_m.h"
#include "Global/Messages/_30_Network/DDRARoutingHeader_m.h"
#include "Nodes/_10_Terminal/_40_Application/Ter_App.h"
#include "Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Global/Utilities/libnorad/cEcef.h"

namespace mac {

Define_Module(Ter_Dlk_KiWan);

// Initialize the static global packet ID
int Ter_Dlk_KiWan::globalPacketID = 0;

Ter_Dlk_KiWan::~Ter_Dlk_KiWan() {
    cancelAndDelete(eventTransmitData);
    cancelAndDelete(eventScheduleTransmission);
    cancelAndDelete(eventTransmitWindowEnd);
    cancelAndDelete(eventStartTransmission);
    cancelAndDelete(eventStartDetection);
    cancelAndDelete(eventDetectionPeriod);
    cancelAndDelete(eventDetectionBatchEnd);
    cancelAndDelete(eventDetectionListenEnd);
}

void Ter_Dlk_KiWan::initialize(int stage) {
    Base_MacProtocol::initialize(stage);
    
    if (stage == INITSTAGE_LOCAL) {
        cModule *phy = getParentModule()->getParentModule()->getSubmodule("phy");
        if (!phy)
            throw cRuntimeError("PHY module not found! Expected submodule named 'phy'");

        // Get the radio inside PHY
        cModule *radioModule = phy->getSubmodule("radio");
        if (!radioModule)
            throw cRuntimeError("Radio submodule not found inside PHY!");

        radio = check_and_cast<IRadio *>(radioModule);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);

        // LoRa parameters
        LoRaSF = par("LoRaSF");
        LoRaTP = par("LoRaTP");
        LoRaCF = par("LoRaCF");
        LoRaBW = par("LoRaBW");
        LoRaCR = par("LoRaCR");

        // Mac parameters
        ttwDuration = par("ttwDuration");
        ttwIdle = par("ttwIdle");
        ttxStart_min = par("ttxStart_min");
        ttxStart_max = par("ttxStart_max");
        ttxIdle_min = par("ttxIdle_min");
        ttxIdle_max = par("ttxIdle_max");

        Ntw = par("Ntw");
        Ntx = par("Ntx");
        
        // Satellite detection parameters
        enableSatDetect = par("enableSatDetect");
        tsdStart_min = par("tsdStart_min");
        tsdStart_max = par("tsdStart_max");
        Ndetect = par("Ndetect");
        NsdSync = par("NsdSync");
        tsyncIdle = par("tsyncIdle");
        tsdIdle = par("tsdIdle");
        satelliteDetectionProbability = par("satelliteDetectionProbability");
        minimumElevation = par("minimumElevation");

        detectInitialListen = par("detectInitialListen");
        detectSuccessExtraMin = par("detectSuccessExtraMin");
        detectSuccessExtraMax = par("detectSuccessExtraMax");
        detectMissExtra = par("detectMissExtra");

        currentTwCount = 0;
        currentTxCount = 0;
        ttxStart = 0;
        ttxIdle = 0;
        windowStartTime = 0;
        
        // Initialize detection state
        currentDetectCount = 0;
        currentSyncCount = 0;
        satelliteDetected = false;
        
        // Initialize counters for statistics
        numTransmissions = 0; // Counter for repetitions sent
        numMessagesSent = 0; // Counter for messages sent
        numDetectionAttempts = 0; // Counter for detection attempts
        numSuccessfulDetections = 0; // Counter for successful detections

        const char *addressString = par("address");
        if (!strcmp(addressString, "auto")) {
            address = MacAddress::generateAutoAddress(); // assign automatic address
            par("address").setStringValue(address.str().c_str()); // change module parameter from "auto" to concrete address
        }
        else {
            address.setAddress(addressString);
        }

        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));
        
        currentState = KIWAN_IDLE;

        //mlBox = check_and_cast<Transmission_Predictor*>(getSystemModule()->getSubmodule("transmissionPredictor"));
        mlBoxAvailable = par("useMLBox").boolValue();
        if( mlBoxAvailable ){
            mlBox = check_and_cast<mlbox::Transmission_Predictor *>(getSubmodule("mlBox"));
        }

    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        this->radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
    }
}

void Ter_Dlk_KiWan::finish() {
    recordScalar("transmittedRepetitions", this->numTransmissions);
    recordScalar("transmittedMessages", this->numMessagesSent); // Record unique messages sent
    recordScalar("detectionAttempts", this->numDetectionAttempts);
    recordScalar("successfulDetections", this->numSuccessfulDetections);

    recordScalar("numTransmissionsApproved", this->numTransmissionsApproved);

}

void Ter_Dlk_KiWan::handleSelfMessage(cMessage *msg) {
    // Handle EndDetectionPeriod message
    if (strcmp(msg->getName(), "EndDetectionPeriod") == 0) {
        this->radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        delete msg;
        return;
    }

    if (debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": handling self message: " << msg->getName() << endl;
        std::cout << "Current State: " << currentState << endl;
    }
    
    // Handle eventStartTransmission in any state - it's the periodic window trigger
    if (msg == this->eventStartTransmission) {
        startTransmissionWindow();
        return;
    }
    
    // Handle eventTransmitWindowEnd in any state - ensures cleanup happens even if
    // detection failed and state returned to IDLE before the window end fired
    if (msg == this->eventTransmitWindowEnd) {
        handleTransmitWindowEnd();
        return;
    }
    
    switch(currentState) {
        case KIWAN_IDLE:
            if (msg == this->eventScheduleTransmission) {
                scheduleTransmission();
            }
            break;
        
        case KIWAN_DETECT:
            if (msg == this->eventStartDetection) {
                startDetectionBatch();
            }
            else if (msg == this->eventDetectionPeriod) {
                performDetection();
            }
            else if (msg == this->eventDetectionListenEnd) {
                handleDetectionListenEnd();
            }
            else if (msg == this->eventDetectionBatchEnd) {
                handleDetectionBatchEnd();
            }
            break;
            
        case KIWAN_TRANSMIT:
            if (msg == this->eventTransmitData) {
                sendData();
            }
            break;
    }
}

void Ter_Dlk_KiWan::handleUpperMessage(cMessage *msg) {
    auto pkt = check_and_cast<Packet *>(msg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    
    auto pktEncap = encapsulate(pkt);

    this->txQueue->enqueuePacket(pktEncap);
    
    // If idle, start transmission procedure immediately
    if (currentState == KIWAN_IDLE) {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": received data while idle, starting transmission immediately" << endl;
        }
        scheduleTransmission();
    }
}


void Ter_Dlk_KiWan::handleLowerMessage(cMessage *msg) {
    // for the moment, no messages from satellite
}

void Ter_Dlk_KiWan::scheduleTransmission() {
    // If a transmission is already in progress (currentTxFrame exists or window events are scheduled),
    // do nothing - the new packet will be handled after the current transmission completes
    if (currentTxFrame != nullptr || eventStartTransmission->isScheduled()) {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() 
                    << ": transmission already in progress, new packet will wait in queue" << endl;
        }
        return;
    }
    
    if (this->txQueue->getNumPackets() > 0) {
        // Start transmission procedure - windows are managed from IDLE state
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": starting transmission procedure with " << Ntw << " windows" << endl;
        }
        // Stay in IDLE state - window management happens here
        this->currentState = KIWAN_IDLE;
        currentTwCount = 0;
        
        // Get the packet to transmit from the queue
        this->popTxQueue();
        
        // Schedule the first transmission window immediately
        EV << "before first scheduleAt"<<endl;
        scheduleAt(simTime(), this->eventStartTransmission);
    } else {
        // No data to send, remain idle
        this->currentState = KIWAN_IDLE;
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": has no data to send, waiting for new data" << endl;
        }
    }
}

void Ter_Dlk_KiWan::startTransmissionWindow() {
    // This is called from IDLE state to start a new transmission window
    // A transmission window contains: satellite detection (if enabled) + data transmission
    // Windows are periodic: each window starts at a fixed interval (ttwDuration + ttwIdle) from the previous
    
    // Cancel any pending events from previous window
    cancelEvent(this->eventTransmitWindowEnd);
    cancelEvent(this->eventTransmitData);
    cancelEvent(this->eventStartDetection);
    cancelEvent(this->eventDetectionPeriod);
    cancelEvent(this->eventDetectionBatchEnd);
    
    if (currentTwCount < Ntw) {
        // Check if we have a frame to transmit
        if (!currentTxFrame) {
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": has no current frame, ending transmission procedure" << endl;
            }
            currentState = KIWAN_IDLE;
            checkForWaitingData();
            return;
        }
        
        // Count the message as sent at the beginning of the first transmission window
        if (currentTwCount == 0) {
            numMessagesSent++;
        }
        
        currentTwCount++;
        
        // Record window start time for periodic scheduling
        windowStartTime = simTime();
        
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": starting transmission window #" << currentTwCount << " of " << Ntw << endl;
        }
        
        // Schedule the next transmission window at fixed interval from this window's start
        // This makes windows periodic regardless of what happens during the window
        if (currentTwCount < Ntw) {
            simtime_t nextWindowTime = windowStartTime + ttwDuration + ttwIdle;
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": scheduling next window #" << (currentTwCount + 1) 
                        << " at time " << nextWindowTime << " (period = " << (ttwDuration + ttwIdle) << "s)" << endl;
            }
            scheduleAt(nextWindowTime, this->eventStartTransmission);
        }
        
        // Schedule the end of this transmission window (for cleanup)
        scheduleAt(windowStartTime + ttwDuration, this->eventTransmitWindowEnd);
        
        // Reset transmission counter for this window
        currentTxCount = 0;
        
        // Check if satellite detection is enabled for this window
        if (enableSatDetect) {
            // Start satellite detection phase for this window
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": starting satellite detection for window #" << currentTwCount << endl;
            }
            startDetection();
        } else {
            // Skip detection, go directly to transmissions in this window
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": starting transmissions in window #" << currentTwCount << " (no detection)" << endl;
            }
            currentState = KIWAN_TRANSMIT;
            startTransmissionsInWindow();
        }
    } else {
        // All transmission windows completed - delete the frame
        this->deleteCurrentTxFrame();
        
        // Check if more data is waiting
        checkForWaitingData();
    }
}

void Ter_Dlk_KiWan::handleTransmitWindowEnd() {
    if (debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": transmission window #" << currentTwCount << " ended (ttwDuration reached)" << endl;
    }
    
    // Cancel any pending transmission events for this window
    cancelEvent(this->eventTransmitData);
    
    // Return to IDLE state
    currentState = KIWAN_IDLE;
    
    // If this was the last window, do cleanup and check for more data
    // (next window is not scheduled for the last window)
    if (currentTwCount >= Ntw) {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": all " << Ntw << " transmission windows completed" << endl;
        }
        this->deleteCurrentTxFrame();
        checkForWaitingData();
    }
    // Otherwise, next window is already scheduled from startTransmissionWindow
}

void Ter_Dlk_KiWan::startTransmissionsInWindow() {
    // This is called after detection succeeds or when detection is disabled
    // We are now in KIWAN_TRANSMIT state
    // Note: Window end (eventTransmitWindowEnd) is already scheduled from startTransmissionWindow
    
    // Reset transmission counter for this window
    currentTxCount = 0;
    
    // Schedule the first transmission after ttxStart delay randomly within ttxStart_min and ttxStart_max
    ttxStart = uniform(ttxStart_min, ttxStart_max);
    if (debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": scheduling first transmission in window #" << currentTwCount 
                << " after " << ttxStart << " seconds" << endl;
    }

    scheduleAt(simTime() + ttxStart, this->eventTransmitData);
}

void Ter_Dlk_KiWan::checkForWaitingData() {
    this->currentState = KIWAN_IDLE;
    if (this->txQueue->getNumPackets() > 0) {
        // More data waiting, start a new transmission procedure
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": has more data waiting after transmission, starting new procedure" << endl;
        }
        currentTwCount = 0;
        scheduleAt(simTime(), this->eventScheduleTransmission);
    } else {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": has no more data, returning to IDLE" << endl;
        }
    }
}

void Ter_Dlk_KiWan::sendData() {
    if (currentTxCount < Ntx) {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": sending transmission #" << (currentTxCount + 1) 
                    << " in window #" << currentTwCount << endl;
        }
        
        EV << "Ter_Dlk_KiWan::sendData" <<endl;
        EV << "time_1= " << simTime().dbl() <<endl;

        this->radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

        // Create a new packet for this transmission instead of duplicating
        // to avoid INET packet chunk representation issues
        auto frameCopy = new Packet(currentTxFrame->getName());
        
        // Get the original frame to copy its data
        const auto& origFrame = currentTxFrame->peekAtFront<KiWanFrame>();
        auto newFrame = makeShared<KiWanFrame>(*origFrame);
        
        // Update the transmission-specific fields
        int repetitionNumber = ((currentTwCount - 1) * Ntx) + (currentTxCount + 1);
        newFrame->setRepetition(repetitionNumber);
        newFrame->setSentAt(simTime().dbl());
        
        // Insert the frame
        frameCopy->insertAtFront(newFrame);
        
        // Copy the payload (everything after the frame)
        auto payloadLength = currentTxFrame->getDataLength() - origFrame->getChunkLength();
        if (payloadLength > b(0)) {
            auto payload = currentTxFrame->peekDataAt(origFrame->getChunkLength(), payloadLength);
            frameCopy->insertAtBack(payload);
        }
        
        // Copy tags from original packet
        frameCopy->copyTags(*currentTxFrame);

        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime()
                      << ": sending frame with ID: " << newFrame->getId() << endl;
        }

        EV << "Ter_Dlk_KiWan::sendData" <<endl;

        EV << "newFrame->getRepetition(): " << newFrame->getRepetition() <<endl;
        EV << "newFrame->getId(): " << newFrame->getId()<< endl;

        auto sequence   = dynamicPtrCast<const SequenceChunk>(frameCopy->peekData());
        int index = 0;
        for (const auto& chunk : sequence->getChunks()) {
            EV << "Chunk #" << index << ": " << chunk->getChunkType()
                    << " - length: " << chunk->getChunkLength() << endl;
            chunk->printToStream(EV, cLog::logLevel, 0);
            EV << "\n -"<<endl;
            index = index +1;
        }
        EV << "-------"<<endl;

        auto transmissionTag = frameCopy->addTagIfAbsent<statistics::CstTransmissionStatisticsTag>();
        satellite::Sat *sat  = check_and_cast<satellite::Sat *>(getSystemModule()->getSubmodule("satellite", 0));
        Ter *ter             = check_and_cast<Ter *>(getParentModule()->getParentModule());
        double loRaTP        = ter->getSubmodule("app")->par("initialLoRaTP").doubleValue();
        int loRaSF           = ter->getSubmodule("app")->par("initialLoRaSF");

        float latDev  = float(ter->getLatitude());
        float longDev = float(ter->getLongitude());
        double elevSat_temp = sat->getElevation(*ter);
        float elevSat = float(elevSat_temp);
        float loraTP  = float((mW(math::dBmW2mW(loRaTP)).get())/1000); // We split this value to put in the same range than dataset, since we use a Scaler this does not affect and should be in the same range.
        float loraSF  = float(loRaSF);

        // COmpute Doppler frequency
        Hz originalFreq = Hz(ter->getSubmodule("app")->par("initialLoRaCF").doubleValue());

        Ter_Mob *ter_mob = check_and_cast<Ter_Mob *>(ter->getSubmodule("mob"));
        Ter_Mob *transmitterTerMobility  = dynamic_cast<mobility::Ter_Mob *>(ter_mob);
        Sat_Mob_SatelliteMobility_Standalone *receiverSatMobility     = dynamic_cast<mobility::Sat_Mob_SatelliteMobility_Standalone *>(getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("mobility"));//(receiverMobility);
        double terminalLatDeg = transmitterTerMobility->getLatitude();
        double terminalLonDeg = transmitterTerMobility->getLongitude();
        double terminalAltKm  = transmitterTerMobility->getCurrentPosition().z / 1000.0; // m -> km
        double satelliteLatDeg = receiverSatMobility->getLatitude();
        double satelliteLonDeg = receiverSatMobility->getLongitude();
        double satelliteAltKm  = receiverSatMobility->getAltitude();


        cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
        cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

        // Use real ECEF velocity from satellite mobility (m/s)
        Coord rxVel;

        if (receiverSatMobility) {
            //EV_DETAIL << "Satellite real velocity (m/s): " << satMobility->getRealVelocity() << endl;
            EV_DETAIL << "Satellite real velocity (m/s): " << receiverSatMobility->getCurrentVelocityEcef() << endl;
            //rxVel = satMobility->getCurrentVelocityEcef();
            rxVel = receiverSatMobility->getCurrentVelocityEcef();
        }

        // Relative position vector in ECEF (km)
        Coord relativePos(
            satelliteEcef.getX() - terminalEcef.getX(),
            satelliteEcef.getY() - terminalEcef.getY(),
            satelliteEcef.getZ() - terminalEcef.getZ()
        );


        double distance = relativePos.length();
        EV_DETAIL << "computeDopplerFrequency: relative position (km): " << relativePos << ", distance: " << distance << " km" << endl;

        // Convert rxVel from m/s to km/s for consistency with relativePos (km)
        Coord rxVelKmPerS = rxVel / 1000.0;

        double relativeVelAlongLOS = (relativePos.x * rxVelKmPerS.x +
                                      relativePos.y * rxVelKmPerS.y +
                                      relativePos.z * rxVelKmPerS.z) / distance;
        // Positive radial velocity => satellite is approaching terminal
        double radialVelocity = -relativeVelAlongLOS; // km/s

        // Doppler: f_rx = f_tx * (c + v_r) / c
        double c = 299792.458; // km/s
        double dopplerShiftedFreqValue = originalFreq.get() * (c + radialVelocity) / c;
        double dopplerShift = dopplerShiftedFreqValue - originalFreq.get();
        EV_INFO << "calculate DopplerShift: " << dopplerShift <<endl;
        EV_INFO << "computeDopplerFrequency: original=" << originalFreq
                << ", radial_vel=" << radialVelocity
                << " km/s, shifted=" << dopplerShiftedFreqValue / 1e6
                << " MHz (shift=" << dopplerShift / 1e6 << " MHz)" << endl;

        float doppler = float(Hz(dopplerShift).get());

        float alt     = float((getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule"))->par("altitude").doubleValue());
        float raan    = float((getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule"))->par("raan").doubleValue());

        //double elevSat_temp  = sat->getElevation(*ter);
        transmissionTag->setElevSat(elevSat_temp);

        EV << "time= " << simTime().dbl() <<endl;
        EV << "latDev=" << latDev
           << ", longDev=" << longDev
           << ", elevSat=" << elevSat
           << ", loraTP=" << loraTP
           << ", loraSF=" << loraSF
           << ", doppler=" << doppler
           << ", alt=" << alt
           << ", raan=" << raan << endl;

        //std::vector<float> myFeatures = std::vector<float>({latDev, longDev, elevSat, loraTP, loraSF, doppler, alt, raan});

        EV << "elevSat_temp= " << elevSat_temp <<endl;

        bool transmit = true;
        if(mlBoxAvailable){
            std::vector<double> myFeatures = {latDev, longDev, elevSat,loraTP, loraSF, doppler,alt, raan};

            // For Transformer
            //std::vector<double> output = mlBox->predict(mlBox->scaleFeatures(myFeatures));

            // For LSTM
            std::vector<double> myFeatures_2 = mlBox->scaleFeatures(myFeatures);
            myFeatures_2.push_back(simTime().dbl());
            std::vector<double> output = mlBox->predict(myFeatures_2);


            transmit = static_cast<bool>(output[0]);
            EV << "After execute model: " << transmit <<endl;
        }

        if(transmit){
            sendDown(frameCopy);

            this->numTransmissionsApproved++;
        }


        this->numTransmissions++;

        // Increment transmission counter
        currentTxCount++;

        // Schedule next transmission if not at limit
        if (currentTxCount < Ntx) {
            // Schedule next transmission after ttxIdle delay randomly
            ttxIdle = uniform(ttxIdle_min, ttxIdle_max);
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": scheduling next transmission in window #" << currentTwCount
                        << " after " << ttxIdle << " seconds" << endl;
            }
            scheduleAt(simTime() + ttxIdle, this->eventTransmitData);
        }

    }
}

void Ter_Dlk_KiWan::sendDown(cMessage *message)
{
    // Send down to physical layer through the standard lowerLayerOut gate
    send(message, "lowerLayerOut");
}


Packet *Ter_Dlk_KiWan::encapsulate(Packet *msg) {
    auto frame = makeShared<KiWanFrame>();
    frame->setKind(UL_DATA);
    frame->setId(globalPacketID); // Use the shared global packet ID
    frame->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);
    frame->setTransmitterAddress(this->getAddress());
    frame->setLoRaSF(LoRaSF);

    // ACHF
    //loraTag->setPower(mW(math::dBmW2mW(loRaTP)));
    //El input es en dBm, lo pasamos a MW. Esto deberia hacerse en la
    //frame->setLoRaTP(LoRaTP);
    frame->setLoRaTP(mW(math::dBmW2mW(LoRaTP)).get());



    frame->setLoRaCR(LoRaCR);
    frame->setLoRaBW(inet::units::values::Hz(LoRaBW));
    frame->setLoRaCF(inet::units::values::Hz(LoRaCF));
    frame->setSrcId(this->getId());

    frame->setQueuedAt(simTime().dbl());

    frame->setChunkLength(b(128));    
    msg->insertAtFront(frame);

    globalPacketID++; // Increment the global packet ID
    return msg;
}

void Ter_Dlk_KiWan::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            this->radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        }
        transmissionState = newRadioTransmissionState;
    }
}

void Ter_Dlk_KiWan::startDetection() {
    currentState = KIWAN_DETECT;
    currentDetectCount = 0;
    satelliteDetected = false;
    
    // Schedule first detection batch after random delay
    double tsdStart = uniform(tsdStart_min, tsdStart_max);
    if (debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": scheduling first detection batch after " << tsdStart << " seconds" << endl;
    }
    scheduleAt(simTime() + tsdStart, this->eventStartDetection);
}

void Ter_Dlk_KiWan::startDetectionBatch() {
    if (currentDetectCount < Ndetect && !satelliteDetected) {
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": starting detection batch #" << (currentDetectCount + 1) << " of " << Ndetect << endl;
        }
        
        currentSyncCount = 0;
        currentDetectCount++;
        
        // Schedule first detection period in this batch
        scheduleAt(simTime(), this->eventDetectionPeriod);
    } else {
        // Detection complete - only proceed if satellite was detected
        if (satelliteDetected) {
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": satellite detected, starting transmissions in window #" << currentTwCount << endl;
            }
            currentState = KIWAN_TRANSMIT;
            startTransmissionsInWindow();
        } else {
            // All detection batches completed without detection - skip this transmission window
            if (debugOutput) {
                std::cout << "Node " << address << " at time " << simTime() << ": all detection batches completed without detection, skipping transmission window #" << currentTwCount << endl;
            }
            // Return to IDLE - next window is already scheduled from startTransmissionWindow
            currentState = KIWAN_IDLE;
        }
    }
}

void Ter_Dlk_KiWan::performDetection() {
    if (currentSyncCount < NsdSync && !satelliteDetected) {
        numDetectionAttempts++;
        
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": performing detection #" << (currentSyncCount + 1) 
                    << " in batch #" << currentDetectCount << endl;
        }
        
        // Set radio to receiver mode for detection
        this->radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        
        // Check if satellite is in view
        bool satelliteInView = isSatelliteInView();

        const double initialListen = detectInitialListen;
        const double successExtraMin = detectSuccessExtraMin;
        const double successExtraMax = detectSuccessExtraMax;
        const double missExtra = detectMissExtra;

        pendingSatelliteInView = satelliteInView;
        pendingDetectionSuccess = false;
        pendingExtraListen = 0.0;
        pendingTotalListen = initialListen;
        pendingRandValue = 0.0;

        if (satelliteInView) {
            // Probabilistic detection after initial listen
            double randValue = uniform(0.0, 1.0);
            pendingRandValue = randValue;
            if (randValue < satelliteDetectionProbability) {
                pendingDetectionSuccess = true;
                pendingExtraListen = uniform(successExtraMin, successExtraMax);
            } else {
                pendingExtraListen = missExtra;
            }
            pendingTotalListen = initialListen + pendingExtraListen;
        }

        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": detection listen for "
                      << pendingTotalListen << " seconds (satelliteInView=" << pendingSatelliteInView
                      << ", rand=" << pendingRandValue << ", success=" << pendingDetectionSuccess << ")" << endl;
        }

        currentSyncCount++;

        // Schedule end of listening for this detection period
        scheduleAt(simTime() + pendingTotalListen, this->eventDetectionListenEnd);
    }
}

void Ter_Dlk_KiWan::handleDetectionListenEnd() {
    // End of listening period for this detection attempt
    this->radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

    if (pendingSatelliteInView && pendingDetectionSuccess) {
        satelliteDetected = true;
        numSuccessfulDetections++;

        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": satellite DETECTED (prob: "
                      << satelliteDetectionProbability << ", rand: " << pendingRandValue << ")" << endl;
        }

        // Cancel any pending detection events
        cancelEvent(this->eventDetectionPeriod);
        cancelEvent(this->eventDetectionBatchEnd);

        currentState = KIWAN_TRANSMIT;
        startTransmissionsInWindow();
        return;
    }

    if (pendingSatelliteInView && !pendingDetectionSuccess && debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": satellite in view but not detected (prob: "
                  << satelliteDetectionProbability << ", rand: " << pendingRandValue << ")" << endl;
    }
    if (!pendingSatelliteInView && debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": no satellite in view (elevation < "
                  << minimumElevation << " deg)" << endl;
    }

    // Schedule next detection period or end batch
    if (currentSyncCount < NsdSync) {
        scheduleAt(simTime() + tsyncIdle, this->eventDetectionPeriod);
    } else {
        handleDetectionBatchEnd();
    }
}

void Ter_Dlk_KiWan::handleDetectionBatchEnd() {
    if (debugOutput) {
        std::cout << "Node " << address << " at time " << simTime() << ": detection batch #" << currentDetectCount << " completed" << endl;
    }
    
    // Turn off radio
    this->radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
    
    if (satelliteDetected) {
        // Satellite was detected, proceed to transmissions in current window
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": satellite detected, starting transmissions in window #" << currentTwCount << endl;
        }
        currentState = KIWAN_TRANSMIT;
        startTransmissionsInWindow();
    } else if (currentDetectCount < Ndetect) {
        // Schedule next detection batch (stay in DETECT state)
        scheduleAt(simTime() + tsdIdle, this->eventStartDetection);
    } else {
        // All detection batches completed without detection - skip this transmission window
        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": all detection batches completed without detection, skipping transmission window #" << currentTwCount << endl;
        }
        // Return to IDLE - next window is already scheduled from startTransmissionWindow
        currentState = KIWAN_IDLE;
    }
}

bool Ter_Dlk_KiWan::isSatelliteInView() {
    // Get terminal's mobility
    cModule *terminalNode = getParentModule()->getParentModule();
    cModule *mobilityModule = terminalNode->getSubmodule("mob"); 
    if (!mobilityModule) {
        EV_WARN << "No mobility module found for terminal" << endl;
        return false;
    }
    
    auto terminalMobility = dynamic_cast<mobility::Ter_Mob *>(mobilityModule);
    if (!terminalMobility) {
        EV_WARN << "Mobility module is not Ter_Mob type" << endl;
        return false;
    }
    
    double terminalLatDeg = terminalMobility->getLatitude();
    double terminalLonDeg = terminalMobility->getLongitude();
    double terminalAltKm = terminalMobility->getCurrentPosition().z / 1000.0; // Convert to km
    
    // Get the radio medium to access all radios
    cModule *networkModule = getSimulation()->getSystemModule();
    cModule *mediumModule = nullptr;
    
    // Search for medium module
    for (cModule::SubmoduleIterator it(networkModule); !it.end(); ++it) {
        cModule *submodule = *it;
        if (strstr(submodule->getFullName(), "medium") != nullptr || 
            strstr(submodule->getNedTypeName(), "Medium") != nullptr) {
            mediumModule = submodule;
            break;
        }
    }
    
    if (!mediumModule) {
        EV_WARN << "Could not find radio medium module" << endl;
        return false;
    }
    
    // Get list of all radios from medium
    auto radioMedium = dynamic_cast<IRadioMedium *>(mediumModule);
    if (!radioMedium) {
        EV_WARN << "Medium module is not IRadioMedium type" << endl;
        return false;
    }
    // Check all radios for satellites using the medium communication cache
    const ICommunicationCache *communicationCache = radioMedium->getCommunicationCache();
    if (!communicationCache) {
        EV_WARN << "Radio medium has no communication cache" << endl;
        return false;
    }

    bool satelliteInView = false;
    communicationCache->mapRadios([&] (const IRadio *radio) {
        if (satelliteInView || radio == nullptr)
            return;

        const IMobility *mobility = radio->getAntenna()->getMobility();
        auto satMobility = dynamic_cast<const mobility::Sat_Mob_SatelliteMobility *>(mobility);
        if (!satMobility)
            return;

        double satelliteLatDeg = satMobility->getLatitude();
        double satelliteLonDeg = satMobility->getLongitude();
        double satelliteAltKm = satMobility->getAltitude();

        // Compute elevation angle using same approach as CustomMedium
        cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
        cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

        const double dx = (satelliteEcef.getX() - terminalEcef.getX()) / 1000.0; // km
        const double dy = (satelliteEcef.getY() - terminalEcef.getY()) / 1000.0; // km
        const double dz = (satelliteEcef.getZ() - terminalEcef.getZ()) / 1000.0; // km

        const double latRad = terminalLatDeg * M_PI / 180.0;
        const double lonRad = terminalLonDeg * M_PI / 180.0;
        const double sinLat = std::sin(latRad);
        const double cosLat = std::cos(latRad);
        const double sinLon = std::sin(lonRad);
        const double cosLon = std::cos(lonRad);

        // Local ENU (East-North-Up) coordinates
        const double east = -sinLon * dx + cosLon * dy;
        const double north = -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz;
        const double up = cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz;

        const double slantRange = std::sqrt(east * east + north * north + up * up);
        const double elevationRad = std::asin(up / slantRange);
        const double elevationDeg = elevationRad * 180.0 / M_PI;

        if (debugOutput) {
            std::cout << "Node " << address << " at time " << simTime() << ": checking satellite at elevation " << elevationDeg << " degrees" << endl;
        }

        if (elevationDeg >= minimumElevation) {
            EV << "Satellite in view with elevation " << elevationDeg << " >= " << minimumElevation << " degrees" << endl;
            satelliteInView = true;
        }
    });

    return satelliteInView;
}

MacAddress Ter_Dlk_KiWan::getAddress() {
    return address;
}

} // namespace mac

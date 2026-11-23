#ifndef Ter_Dlk_KiWan_H
#define Ter_Dlk_KiWan_H

#include <omnetpp.h>
#include <array>

#include "Global/Base/_20_Data_Link/Base_MacProtocol.h"
#include "Global/Messages/_20_Data_Link/KiWanMacFrame_m.h"
#include "Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.h"
#include "inet/linklayer/common/MacAddress.h"
#include "inet/common/ModuleAccess.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/queueing/contract/IPacketQueue.h"
#include "inet/common/Protocol.h"
#include "Nodes/_10_Terminal/_60_Mobility/Ter_Mob.h"
#include "Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.h"
#include "Global/Utilities/libnorad/cEcef.h"
//#include "Global/Transmission_Predictor/Transmission_Predictor.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_Analytical.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_BiLSTM.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_MLP.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_TwoBranches.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_MLP_KD.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_MLP_SFTP.h"
#include "Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor_MLP_Policy_SFTP.h"


using namespace mlbox;
using namespace omnetpp;
using namespace inet;

namespace mac {

class Ter_Dlk_KiWan : public Base_MacProtocol {
    public:

        virtual ~Ter_Dlk_KiWan();
        virtual void initialize(int stage) override;
        virtual void finish() override;

        virtual void handleSelfMessage(cMessage *msg) override;
        virtual void handleUpperMessage(cMessage *msg) override;
        virtual void handleLowerMessage(cMessage *msg) override;
        virtual void sendDown(cMessage *msg) override;
        MacAddress getAddress() override;
    
    protected:
        enum Ter_Dlk_KiWanState {
            KIWAN_IDLE,         // Default idle state
            KIWAN_DETECT,       // Satellite detection phase
            KIWAN_TRANSMIT,     // Sending data
        };
        Ter_Dlk_KiWanState currentState = KIWAN_IDLE;

        // FSM Events
        cMessage *eventScheduleTransmission = new cMessage("Schedule_Transmission");
        cMessage *eventTransmitData = new cMessage("Transmit_Data");
        cMessage *eventTransmitWindowEnd = new cMessage("TransmitWindowEnd");
        cMessage *eventStartTransmission = new cMessage("StartTransmission");
        cMessage *eventStartDetection = new cMessage("Start_Detection");
        cMessage *eventDetectionPeriod = new cMessage("Detection_Period");
        cMessage *eventDetectionBatchEnd = new cMessage("Detection_Batch_End");
        cMessage *eventDetectionListenEnd = new cMessage("Detection_Listen_End");

        // LoRa parameters
        int LoRaSF;
        int LoRaCR;
        double LoRaTP;
        double LoRaCF;
        int LoRaBW;


        // ACHF
        bool randomizeSF_TP;
        int SF_min;
        int SF_max;
        int TP_min;//dbm
        int TP_max;//dbm

        bool useMixFile = false;
        int mixId = -1;
        std::array<double, 3> pSF{};
        std::array<double, 3> pTP{};
        void loadMix();
        int drawWeighted(const std::array<double, 3>& p, double& prob);

        bool useStopRule;
        double stopTarget;
        double stopGamma;
        double cumMissProb = 1.0; // Π(1 − γ·p) de las tx ya enviadas de este mensaje
        double numSkippedByStopRule = 0;
        double sumApprovedProb = 0;
        double coindOdds;
        int numMLApprovedBelowMask = 0;

        // ACHF - moneda despues del filtro del MLP - KD
        bool useMLCoin = false;
        double mlCoinProb = 0.5;
        int mlCoinRng = 1;
        double numSkippedByMLCoin = 0;   // slots aprobados por el MLP que la moneda descarto

        // Baselines FV_k / RV_k
        int visBudget = 0;      // Maximum transmissions to transmit, 0 disabled
        std::string visPolicy; // first, random
        int visRng = 1;
        double visProb = 0.5; // RV - probabilida de transmitir en cada slot visible
        int visSentInMsg = 0;        // copias ya transmitidas del mensaje actual
        long numVisUnderBudget = 0;  // mensajes con min(k, visibles) > copias enviadas (FV siempre es 0, RV efecto moneda)

        // Mac parameters
        double ttwDuration;     // Duration of transmission window
        double ttwIdle;         // Idle time between transmission windows
        double ttxStart_min;    // Minimum time to start transmission in a transmission window
        double ttxStart_max;    // Maximum time to start transmission in a transmission window
        double ttxIdle_min;     // Minimum idle time between transmissions
        double ttxIdle_max;     // Maximum idle time between transmissions

        int Ntw;                // Number of transmission windows
        int Ntx;                // Number of transmissions per window
        
        // Satellite detection parameters
        bool enableSatDetect;   // Enable satellite detection mechanism
        double tsdStart_min;    // Minimum time before first detection period
        double tsdStart_max;    // Maximum time before first detection period
        int Ndetect;            // Number of detection batches
        int NsdSync;            // Number of detection periods per batch
        double tsyncIdle;       // Idle time between detection periods in a batch
        double tsdIdle;         // Idle time between detection batches
        double satelliteDetectionProbability; // Probability of detecting satellite when in view
        double minimumElevation; // Minimum elevation angle for satellite detection

        // Satellite detection listen timings
        double detectInitialListen;      // Initial listen duration
        double detectSuccessExtraMin;    // Min extra listen when detected
        double detectSuccessExtraMax;    // Max extra listen when detected
        double detectMissExtra;          // Extra listen when miss in view

        MacAddress address;


        inet::physicallayer::IRadio *radio = nullptr;
        IRadio::TransmissionState transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;

        // Essential helpers
        virtual void sendData();
        virtual void scheduleTransmission();
        virtual void startTransmissionWindow();
        virtual void startTransmissionsInWindow();
        virtual void handleTransmitWindowEnd();
        virtual void checkForWaitingData();
        
        // Satellite detection helpers
        virtual void startDetection();
        virtual void startDetectionBatch();
        virtual void performDetection();
        virtual void handleDetectionListenEnd();
        virtual void handleDetectionBatchEnd();
        virtual bool isSatelliteInView();

        virtual Packet *encapsulate(Packet *msg);

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;

    private: 
        static int globalPacketID;  // Shared packet ID for all nodes
        int currentTwCount;         // Current transmission window count
        int currentTxCount;         // Current transmission count in the window
        // ACHF
        int currentTxCountML;

        double ttxStart;            // Time to start transmission
        double ttxIdle;             // Idle time between transmissions
        simtime_t windowStartTime;  // Start time of current transmission window
        
        // Detection state
        int currentDetectCount;     // Current detection batch count
        int currentSyncCount;       // Current detection period count in batch
        bool satelliteDetected;     // Flag indicating if satellite was detected
        bool pendingSatelliteInView = false;
        bool pendingDetectionSuccess = false;
        double pendingExtraListen = 0.0;
        double pendingTotalListen = 0.0;
        double pendingRandValue = 0.0;

        bool debugOutput = false;           // Debug output flag

        // Statistics
        int numTransmissions = 0;   // Count of repetitions sent
        int numMessagesSent = 0; // Count of messages sent
        int numDetectionAttempts = 0; // Count of detection attempts
        int numSuccessfulDetections = 0; // Count of successful detections
        
        // ACHF
        double computeReceptionPower();
        double computeReceptionPower(int lora_TP_dbm);

        // statistics for ML
        int numTransmissionsApproved = 0;
        //int numTransmissionsApprovedTransformer = 0;
        //int numTransmissionsApprovedBiLSTM = 0;

        cOutVector vector_preds_BiLSTM;
        cOutVector vector_preds_Transformer;
        cOutVector vector_preds_Analytical;
        cOutVector vector_preds_MLP;
        cOutVector vector_msgVisiblesOpp; // Numde oportunidades con elevSat>=10
        int msgVisibleOpp = 0;
        int msgPolicyTx   = 0;     // n_prev: veces que el nodo transmitio el mensaje actual (politica con memoria)
        long numPolicyRepeats = 0; // transmisiones de la politica con n_prev >= 1 (repeticiones), para revisar

        int numOppBelowMask = 0;   // oportunidades con elevSat < 10 (todas, incluido el mensaje abierto al final)
        int numSkippedByMinProbability = 0; // usado por SF/TP model


        bool mlBoxAvailableMLP_Policy_SFTP = false;
        int  policyRng = 1;
        long numPolicyNoTx = 0;     // veces que la politica sorteo "no transmitir" (con elevSat >= 10)
    protected:
        Transmission_Predictor *mlBox;
        Transmission_Predictor_BiLSTM *mlBoxBiLSTM;
        Transmission_Predictor_Analytical *predAnalytical;
        Transmission_Predictor_MLP *mlBoxMLP;
        Transmission_Predictor_TwoBranches *mlBoxTwoBranches;
        Transmission_Predictor_MLP_KD *mlBoxMLPKD;
        Transmission_Predictor_MLP_SFTP *mlBoxMLP_SFTP;
        Transmission_Predictor_MLP_Policy_SFTP *mlBoxMLP_Policy_SFTP = nullptr;

        bool mlBoxAvailable = false;
        bool mlBoxAvailableBiLSTM = false;
        bool analyticalAvailable = false;
        bool mlBoxAvailableMLP = false;
        bool mlBoxAvailableTwoBranches = false;
        bool mlBoxAvailableMLPKD = false;
        bool mlBoxAvailableMLP_SFTP = false;

        bool useCoin = false;

        // ACHF - Calculate the inputs for MLP_SFTP
        struct ML_SFTP_Input {
            double dopplerShif, elevSat, latDev, longDev, alt, raan;
        };
        ML_SFTP_Input calculate_ML_SFTP_inputs();
        // ACHF - END

}; // namespace mac

}
#endif

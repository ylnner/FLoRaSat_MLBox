/*
 * KD_TransmissionStatistics.cc
 *
 *  Created on: Aug 24, 2026
 *      Author: ylnner
 */




# include "KD_TransmissionStatistics.h"

namespace statistics{
Define_Module(KD_TransmissionStatistics);


KD_TransmissionStatistics::KD_TransmissionStatistics(){

}

KD_TransmissionStatistics::~KD_TransmissionStatistics(){
    fileStats.close();
}

void KD_TransmissionStatistics::initialize(){
    // create folder structure
    std::string baseBath = "results/KD_TransmissionStatistics/KD_TransmissionStatistics";
    if (mkdir(baseBath.c_str(), 0777) && errno != EEXIST) {
        error("mkdir failed: %s", strerror(errno));
    }


    //int repetition              = getEnvir()->getConfigEx()->getActiveRunNumber();

    const char* repStr = getEnvir()->getConfigEx()->getVariable("repetition");
    int repetition = repStr ? std::stoi(repStr) : 0;

    double altitude  = (getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule")->par("altitude"));//std::round(sat->getAltitude());
    double raan      = (getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule")->par("raan"));//std::round(sat->getRAAN());
    int stats_altitude = int(altitude);
    int stats_raan = int(raan);

    EV << "stats_altitude: " << stats_altitude <<endl;
    EV << "stats_raan: " << stats_raan <<endl;

    std::string train_or_test   = getSystemModule()->par("train_or_test");


    std::string otherFileName = "_alt_" + std::to_string(stats_altitude) +
                                "_raan_" + std::to_string(stats_raan) +
                                "_seed_" + std::to_string(repetition);

    // Processing filename
    /*
    std::string filenameStats = "";
    if (train_or_test == "train") {
        filenameStats = baseBath + "_train" + otherFileName + ".csv";
    }else if(train_or_test == "test") {
        filenameStats = baseBath + "_test" + otherFileName + ".csv";
    }else if(train_or_test == "val") {
        filenameStats = baseBath + "_val" + otherFileName + ".csv";
    }else{
        filenameStats = baseBath + otherFileName + ".csv";
    }
    */
    std::string filenameStats = "";
    filenameStats = baseBath + "_" + train_or_test + otherFileName + ".csv";

    // write header for stats file
    std::ofstream fileStatsTmp;
    fileStatsTmp.open(filenameStats);

    signal_TranmissionStatisticsCorrect = cComponent::registerSignal("Sat_Dsl_Phy_TranmissionStatisticsCorrect");
    signal_TranmissionStatisticsIgnoring = cComponent::registerSignal("Sat_Dsl_Phy_TranmissionStatisticsIgnoring");

    signal_LoRaReceiver = cComponent::registerSignal("LoRa_Receiver_TranmissionStatistics");

    getSystemModule()->subscribe(signal_TranmissionStatisticsCorrect, this);
    getSystemModule()->subscribe(signal_TranmissionStatisticsIgnoring, this);


    fileStatsTmp <<"pid,srcId,dstId,srcSat,dstSat,tx_start,tx_duration,repetition,latDev,longDev,satId,elevSat,dopplerShift,loraTP,loraCF,loraSF,loraBW,loraCR,rssi_dbm,snr_db,rx_power_dbm,rx_power_dbm_at_dev,collision_flag,num_colliding_packets,rcvOk";
    fileStatsTmp << "\n";
    fileStatsTmp.close();

    fileStats.open(filenameStats, std::ios_base::app);
}

void KD_TransmissionStatistics::handleMessage(cMessage *msg){
    error("Unsupported");
}

void KD_TransmissionStatistics::recordPacket(inet::Packet *packet){
    Enter_Method("recordPacket");
    EV << "TransmissionStatistics::recordPacket" <<endl;

    auto routingTag      = packet->getTag<routing::CstRoutingTag>();
    auto transmissionTag = packet->getTag<statistics::CstTransmissionStatisticsTag>();
    int pid = transmissionTag->getPacketId();


    int srcSat      = routingTag->getSrcSat();
    int dstSat      = routingTag->getDstSat();
    int srcGsOrDev  = routingTag->getSrcGsOrDev();
    int dstGsOrDev  = routingTag->getDstGsOrDev();
    int srcId       = routingTag->getSrcId();
    int dstId       = routingTag->getDstId();

    double created  = packet->getTag<inet::CreationTimeTag>()->getCreationTime().dbl();
    double recorded = simTime().dbl();

    // "pid, srcId, dstId, srcSat, dstSat,
    // tx_start, tx_duration, replica_idx,
    // latDev,longDev,satId,elevSat,dopplerShift,
    // loraTP,loraCF,loraSF,loraBW,loraCR,
    // rssi_dbm, snr_db, rx_power_dbm, collision_flag, num_colliding_packets,
    // rcvOk";

    // tx_duration,

    //pid,srcId,dstId,srcSat,dstSat,tx_start,tx_duration,repetition,latDev,longDev,satId,elevSat,dopplerShift,
    //loraTP,loraCF,loraSF,loraBW,loraCR,rssi_dbm,snr_db,rx_power_dbm,
    //collision_flag,num_colliding_packets,rcvOk
    double tx_start             = core::roundTo(transmissionTag->getTxStart(),6);
    double tx_duration          = core::roundTo(transmissionTag->getTxDuration(),6);
    int repetition              = transmissionTag->getRepetition();
    double latDev               = core::roundTo(transmissionTag->getLatDev(),6);
    double longDev              = core::roundTo(transmissionTag->getLongDev(),6);
    int satId                   = transmissionTag->getSatId();
    double elevSat              = core::roundTo(transmissionTag->getElevSat(),6);
    double dopplerShift         = core::roundTo(transmissionTag->getDoppler(),6);
    double loraTP               = core::roundTo(transmissionTag->getLoraTP(),6);
    double loraCF               = core::roundTo(transmissionTag->getLoraCF(),6);
    int loraSF                  = transmissionTag->getLoraSF();
    double loraBW               = core::roundTo(transmissionTag->getLoraBW(),6);
    int loraCR                  = transmissionTag->getLoraCR();
    double rssi_dbm             = core::roundTo(transmissionTag->getRssi_dbm(),6);
    double snr_db               = core::roundTo(transmissionTag->getSnr_db(),6);
    double rx_power_dbm         = core::roundTo(transmissionTag->getRx_power_dbm(),6);
    int collision_flag          = transmissionTag->getCollision_flag();
    int num_colliding_packets   = transmissionTag->getNum_colliding_packets();
    int rcvOk                   = transmissionTag->getRcvOk();
    double rx_power_dbm_at_dev  = core::roundTo(transmissionTag->getRx_power_dbm_dev(),6);

    // write statistics
    fileStats << pid;
    fileStats << ","<< srcId;
    fileStats << ","<< dstId;
    fileStats << "," << srcSat;
    fileStats << "," << dstSat;
    fileStats << "," << tx_start;
    fileStats << "," << tx_duration;
    fileStats << "," << repetition;
    fileStats << "," << latDev;
    fileStats << "," << longDev;
    fileStats << "," << satId;
    fileStats << "," << elevSat;
    fileStats << "," << dopplerShift;
    fileStats << "," << loraTP;
    fileStats << "," << loraCF;
    fileStats << "," << loraSF;
    fileStats << "," << loraBW;
    fileStats << "," << loraCR;
    fileStats << "," << rssi_dbm;
    fileStats << "," << snr_db;
    fileStats << "," << rx_power_dbm;
    fileStats << "," << rx_power_dbm_at_dev;
    fileStats << "," << collision_flag;
    fileStats << "," << num_colliding_packets;
    fileStats << "," << rcvOk;
    fileStats << "\n";
}

void KD_TransmissionStatistics::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details){
    EV << "TransmissionStatistics::receiveSignal" <<endl;
    EV << "==================================================" <<endl;
    EV << "signalID: " << signalID <<endl;

    if(signalID == signal_TranmissionStatisticsCorrect || signalID == signal_TranmissionStatisticsIgnoring){
        cValueMap *statsMap = check_and_cast<cValueMap *>(obj);

        double minPower     = statsMap->get("minPower_dBm").doubleValue();
        double sensitivity  = statsMap->get("sensitivity_dBm").doubleValue();
        double duration     = statsMap->get("duration").doubleValue();
        double doppler_map  = statsMap->get("doppler").doubleValue();

        cPacket *aux_frame  = check_and_cast<cPacket *>(statsMap->get("macFrame").objectValue());
        Packet *pkt         = check_and_cast<Packet *>(aux_frame);

        const auto& frame       = pkt->peekAtFront<messages::LoRaPhyPreamble>();
        auto transmissionTag    = pkt->addTagIfAbsent<statistics::CstTransmissionStatisticsTag>();
        transmissionTag->setLoraBW(frame->getBandwidth().get());
        transmissionTag->setLoraCF(frame->getCenterFrequency().get());
        transmissionTag->setLoraCR(frame->getCodeRendundance());
        transmissionTag->setLoraSF(frame->getSpreadFactor());
        transmissionTag->setLoraTP(frame->getPower().get());
        transmissionTag->setTxDuration(duration);
        transmissionTag->setSensitivity_dbm(sensitivity);
        transmissionTag->setMinPower_dbm(minPower);

        if(signalID == signal_TranmissionStatisticsCorrect){
            transmissionTag->setRcvOk(1);
        }else if(signalID == signal_TranmissionStatisticsIgnoring){
            transmissionTag->setRcvOk(0);
        }

        auto routingTag     = pkt->addTagIfAbsent<routing::CstRoutingTag>();
        int srcId           = routingTag->getSrcId();
        terminal::Ter *ter  = check_and_cast<terminal::Ter *>(getSystemModule()->getSubmodule("terminal", srcId));


        satellite::Sat *sat                                     = check_and_cast<satellite::Sat *>(source->getParentModule()->getParentModule());
        mobility::Sat_Mob_SatelliteMobility_Standalone *sat_mob = check_and_cast<mobility::Sat_Mob_SatelliteMobility_Standalone *>(sat->getSubmodule("mobility"));


        int satIndex    = sat->getId();
        double snrd_db  = -999;

        if(transmissionTag->getRx_power_dbm() != -999){
            snrd_db  = transmissionTag->getRx_power_dbm() - transmissionTag->getNoise_dbm();
        }


        transmissionTag->setSatId(satIndex);
        transmissionTag->setDoppler(doppler_map);
        transmissionTag->setSnr_db(snrd_db);

        //double minPower     = statsMap->get("minPower_dBm").doubleValue();
        //double sensitivity  = statsMap->get("sensitivity_dBm").doubleValue();
        //////////////////////////////////////recordPacket(pkt, minPower, sensitivity, duration);
        recordPacket(pkt);
    }
}


}

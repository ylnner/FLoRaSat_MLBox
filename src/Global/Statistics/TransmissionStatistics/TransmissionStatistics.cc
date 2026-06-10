/*
 * TransmissionStatistics.cc
 *
 *  Created on: Feb 16, 2026
 *      Author: ylnner
 */



#include "TransmissionStatistics.h"

namespace statistics{

Define_Module(TransmissionStatistics);

TransmissionStatistics::TransmissionStatistics(){

}

TransmissionStatistics::~TransmissionStatistics(){
    fileStats.close();
}


void TransmissionStatistics::initialize(){
    // create folder structure
    std::string baseBath = "results/TransmissionStatistics";
    if (mkdir(baseBath.c_str(), 0777) && errno != EEXIST) {
        error("mkdir failed: %s", strerror(errno));
    }
    // INorad* noradModule;
    // noradModule = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
    //Sat_Mob_NoradA* noradModule = check_and_cast<Sat_Mob_NoradA *>(getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule"));

    //int repetition = getEnvir()->getConfigEx()->getActiveRun().repetition;
    int repetition = getEnvir()->getConfigEx()->getActiveRunNumber();
    //int seedSet = getEnvir()->getConfigEx()->getActiveRunConfig()->getConfigValueAsInt("seed-set");
    //EV << "Current seed-set: " << seedSet << endl;
    EV << "repetition: " << repetition <<endl;

    int stats_altitude = getSystemModule()->par("stats_altitude"); //default(600);
    int stats_raan = getSystemModule()->par("stats_raan");
    std::string train_or_test = getSystemModule()->par("train_or_test");


    std::string otherFileName = "_alt_" + std::to_string(stats_altitude) +
                                "_raan_" + std::to_string(stats_raan) +
                                "_seed_" + std::to_string(repetition);
    /*
    if (train_or_test != ''){
        std::string filenameStats = baseBath + otherFileName + ".csv";
    }
    */

    std::string filenameStats = "";
    if (train_or_test == "train") {
        filenameStats = baseBath + "_train_" + otherFileName + ".csv";
    }else if(train_or_test == "test") {
        filenameStats = baseBath + "_test_" + otherFileName + ".csv";
    }else{
        filenameStats = baseBath + otherFileName + ".csv";
    }



    // write header for stats file
    std::ofstream fileStatsTmp;
    fileStatsTmp.open(filenameStats);


    signal_TranmissionStatisticsCorrect = cComponent::registerSignal("Sat_Dsl_Phy_TranmissionStatisticsCorrect");
    signal_TranmissionStatisticsIgnoring = cComponent::registerSignal("Sat_Dsl_Phy_TranmissionStatisticsIgnoring");

    signal_LoRaReceiver = cComponent::registerSignal("LoRa_Receiver_TranmissionStatistics");

    //cModule *network = getParentModule(); // Obtiene la red principal
    getSystemModule()->subscribe(signal_TranmissionStatisticsCorrect, this);
    getSystemModule()->subscribe(signal_TranmissionStatisticsIgnoring, this);

    //fileStatsTmp << "pid,time,srcId,dstId,srcSat,dstSat,latDev,longDev,satId,elevSat,doppler";
    fileStatsTmp << "pid,time,srcId,dstId,srcSat,dstSat,latDev,longDev,loraTP,loraCF,loraSF,loraBW,loraCR,satId,elevSat,doppler,rcvOk,minPowerdBm,sensitivitydBm,duration";

    fileStatsTmp << "\n";
    fileStatsTmp.close();

    fileStats.open(filenameStats, std::ios_base::app);
}

void TransmissionStatistics::handleMessage(cMessage *msg){
    error("Unsupported");
}


void TransmissionStatistics::recordPacket(inet::Packet *packet, double minPower_dBm, double sensitivity_dBm, double duration){
    Enter_Method("recordPacket");

    auto routingTag      = packet->getTag<routing::CstRoutingTag>();
    auto transmissionTag = packet->getTag<statistics::CstTransmissionStatisticsTag>();

    EV << "TransmissionStatistics::recordPacket" <<endl;
    msgid_t pid = packet->getId();


    int srcSat = routingTag->getSrcSat();
    int dstSat = routingTag->getDstSat();

    int srcGsOrDev = routingTag->getSrcGsOrDev();
    int dstGsOrDev = routingTag->getDstGsOrDev();

    int srcId = routingTag->getSrcId();
    int dstId = routingTag->getDstId();

    double created  = packet->getTag<inet::CreationTimeTag>()->getCreationTime().dbl();
    double recorded = simTime().dbl();

    // write statistics
    // id
    fileStats << pid;
    fileStats << ", "<< core::roundTo(simTime().dbl(), 6);
    fileStats << ", "<< srcId;
    fileStats << ", "<< dstId;
    fileStats << "," << srcSat;
    fileStats << "," << dstSat;
    fileStats << "," << core::roundTo(transmissionTag->getLatDev(),6);
    fileStats << "," << core::roundTo(transmissionTag->getLongDev(),6);

    fileStats << "," << core::roundTo(transmissionTag->getLoraTP(),6);
    fileStats << "," << core::roundTo(transmissionTag->getLoraCF(),6);
    fileStats << "," << transmissionTag->getLoraSF();
    fileStats << "," << core::roundTo(transmissionTag->getLoraBW(),6);
    fileStats << "," << transmissionTag->getLoraCR();

    fileStats << "," << transmissionTag->getSatId();
    fileStats << "," << core::roundTo(transmissionTag->getElevSat(),6);
    fileStats << "," << core::roundTo(transmissionTag->getDoppler(),6);

    fileStats << "," << transmissionTag->getRcvOk();

    fileStats << "," << minPower_dBm;
    fileStats << "," << sensitivity_dBm;
    fileStats << "," << duration;
    fileStats << "\n";


    // fileStatsTmp << "pid,time,srcId,dstId,srcSat,dstSat,latDev,loraTP,loraCF,loraSF,loraBW,loraCR,longDev,satId,elevSat,doppler,rcvOk";
    /*
        double loraTP    = 0;
        double loraCF   = 0;
        int loraSF      = 0;
        double loraBW   = 0;
        int loraCR      = 0;

        int rcvOk       = -1;
     * */
}


void TransmissionStatistics::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details){
    EV << "TransmissionStatistics::receiveSignal" <<endl;
    EV << "==================================================" <<endl;
    EV << "signalID: " << signalID <<endl;

    /*if(signalID == signal_LoRaReceiver){
        auto *map = check_and_cast<cValueMap *>(obj);
        double minReceptionPower_dBm = map->get("minPower_dBm").doubleValue();
        double sensitivity_dBm       = map->get("sensitivity_dBm").doubleValue();

        EV << "receiveSignal minReceptionPower_dBm: " << minReceptionPower_dBm <<endl;
        EV << "receiveSignal sensitivity_dBm: " << sensitivity_dBm <<endl;

        delete map;

    }*/


    if(signalID == signal_TranmissionStatisticsCorrect || signalID == signal_TranmissionStatisticsIgnoring){
        // 1. Convertir el objeto recibido a cValueMap
        cValueMap *statsMap = check_and_cast<cValueMap *>(obj);

        // 2. Extraer los valores básicos (double, simtime_t)
        double minPower = statsMap->get("minPower_dBm").doubleValue();
        double sensitivity = statsMap->get("sensitivity_dBm").doubleValue();
        double duration = statsMap->get("duration").doubleValue();
        double doppler_map = statsMap->get("doppler").doubleValue();

        // 3. Extraer el objeto (macFrame)
        // Usamos objectValue() y luego check_and_cast
        cPacket *aux_frame = check_and_cast<cPacket *>(statsMap->get("macFrame").objectValue());

        // 4. Ahora puedes usar los datos para tus estadísticas
        EV << "Recibida señal de: " << source->getFullPath() << endl;
        EV << "Paquete: " << aux_frame->getName() << endl;
        EV << "Power: " << minPower << " dBm" << endl;
        EV << "Sensitivity: "<< sensitivity << " dBm" << endl;
        EV << "Duration: " << duration <<endl;
        EV << "Doppler stats map: "<< doppler_map<<endl;

        //Packet *pkt = check_and_cast<Packet *>(obj);
        Packet *pkt = check_and_cast<Packet *>(aux_frame);
        auto sequence   = dynamicPtrCast<const SequenceChunk>(pkt->peekData());
        int index = 0;
        for (const auto& chunk : sequence->getChunks()) {
            EV << "Chunk #" << index << ": " << chunk->getChunkType()
                    << " - length: " << chunk->getChunkLength() << endl;
            chunk->printToStream(EV, cLog::logLevel, 0);
            EV << "\n -"<<endl;
            index = index +1;
        }
        EV << "-------"<<endl;


        const auto& frame = pkt->peekAtFront<messages::LoRaPhyPreamble>();
        auto transmissionTag = pkt->addTagIfAbsent<statistics::CstTransmissionStatisticsTag>();
        transmissionTag->setLoraBW(frame->getBandwidth().get());
        transmissionTag->setLoraCF(frame->getCenterFrequency().get());
        transmissionTag->setLoraCR(frame->getCodeRendundance());
        transmissionTag->setLoraSF(frame->getSpreadFactor());
        transmissionTag->setLoraTP(frame->getPower().get());


        if(signalID == signal_TranmissionStatisticsCorrect){
            transmissionTag->setRcvOk(1);
        }else if(signalID == signal_TranmissionStatisticsIgnoring){
            transmissionTag->setRcvOk(0);
        }

        auto routingTag = pkt->addTagIfAbsent<routing::CstRoutingTag>();

        int srcId = routingTag->getSrcId();
        terminal::Ter *ter = check_and_cast<terminal::Ter *>(getSystemModule()->getSubmodule("terminal", srcId));

        satellite::Sat *sat = check_and_cast<satellite::Sat *>(source->getParentModule()->getParentModule());
        mobility::Sat_Mob_SatelliteMobility_Standalone *sat_mob = check_and_cast<mobility::Sat_Mob_SatelliteMobility_Standalone *>(sat->getSubmodule("mobility"));

        /*
        double terminalLatDeg = ter->getLatitude();
        double terminalLonDeg = ter->getLongitude();
        double terminalAltKm  = ter->getAltitude();

        double satelliteLatDeg = sat->getLatitude();
        double satelliteLonDeg = sat->getLongitude();
        double satelliteAltKm  = sat->getAltitude();

        cEcef terminalEcef(terminalLatDeg, terminalLonDeg, terminalAltKm);
        cEcef satelliteEcef(satelliteLatDeg, satelliteLonDeg, satelliteAltKm);

        // Compute relative position vector in ECEF coordinates (km)
        Coord relativePos(
            satelliteEcef.getX() - terminalEcef.getX(),
            satelliteEcef.getY() - terminalEcef.getY(),
            satelliteEcef.getZ() - terminalEcef.getZ()
        );
        double distance = relativePos.length();
        EV_DETAIL << "Relative position vector (km): " << relativePos << ", Distance: " << distance << " m" << endl;

        // rxVel is in m/s from ECEF, relativePos is in km, need consistent units
        // Convert rxVel to km/s for calculation
        Coord rxVel = sat_mob->getCurrentVelocityEcef();
        Coord rxVelKmPerS = rxVel / 1000.0;

        EV << "rxVel: " << rxVel.getX() << " - " << rxVel.getY() << " - " << rxVel.getZ() <<endl;

        double relativeVelAlongLOS = (relativePos.x * rxVelKmPerS.x + relativePos.y * rxVelKmPerS.y + relativePos.z * rxVelKmPerS.z) / distance;
        double radialVelocity = -relativeVelAlongLOS;  // km/s, positive if approaching (negated because relativePos points away from terminal)
        units::values::Hz originalFreq = Hz(868e6); //tag->getCenterFrequency();
        // Doppler shift: f_received = f_transmitted * (c + v_r) / c
        // where v_r is radial velocity (positive when approaching)
        double c = 299792.458; // Speed of light in km/s
        double dopplerShiftedFreqValue = originalFreq.get() * (c + radialVelocity) / c;
        double dopplerShift = dopplerShiftedFreqValue - originalFreq.get();
        EV << "dopplerShift calculate in TransmissionStatistics: "<< dopplerShift<<endl;
        */

        int satIndex = sat->getId();

        transmissionTag->setSatId(satIndex);

        // Moving the calculation of elevation at transmission section.
        ////transmissionTag->setElevSat(sat->getElevation(*ter));
        transmissionTag->setDoppler(doppler_map);

        recordPacket(pkt, minPower, sensitivity, duration);

    }

    // falta agregar delete delete signals or packets received; // MUY IMPORTANTE
}

}

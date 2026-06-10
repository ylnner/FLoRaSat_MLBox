/*
 * PacketRecorder.h
 *
 * Created on: Aug 07, 2023
 *     Author: Robin Ohs
 */

#include "PacketRecorder.h"

namespace statistics {

Define_Module(PacketRecorder);

/** Class */
PacketRecorder::PacketRecorder() {
}

PacketRecorder::~PacketRecorder() {
    fileStats.close();
    fileRoutes.close();
}

/** OMNETPP */
void PacketRecorder::initialize() {
    // read configuration parameters
    std::string constName = par("constName").stdstringValue();
    std::string algName = par("algName").stdstringValue();
    std::string simName = par("simName").stdstringValue();
    std::string repetition = par("repetition").stdstringValue();

    // create folder structure
    std::string baseBath = "results/" + algName;
    if (mkdir(baseBath.c_str(), 0777) && errno != EEXIST) {
        error("mkdir failed: %s", strerror(errno));
    }
    baseBath = baseBath + "/" + constName;
    if (mkdir(baseBath.c_str(), 0777) && errno != EEXIST) {
        error("mkdir failed: %s", strerror(errno));
    }
    baseBath = baseBath + "/" + simName;
    if (mkdir(baseBath.c_str(), 0777) && errno != EEXIST) {
        error("mkdir failed: %s", strerror(errno));
    }
    baseBath = baseBath + "/" + repetition;
    std::string filenameQueue = baseBath + ".sats.csv";
    std::string filenameStats = baseBath + ".stats.csv";
    std::string filenameRoutes = baseBath + ".routes.csv";

    // write header for queue file
    std::ofstream fileQueueTmp;
    fileQueueTmp.open(filenameQueue);
    fileQueueTmp << "sat_id,timestamp,queue_size";
    fileQueueTmp << "\n";
    fileQueueTmp.close();

    // write header for stats file
    std::ofstream fileStatsTmp;
    fileStatsTmp.open(filenameStats);
    //fileStatsTmp << "pid,type,dropReason,size,srcSat,dstSat,srcGs,dstGs,hops,created,recorded,queueDelay,procDelay,transDelay,propDelay";
    fileStatsTmp << "pid,type,dropReason,size,srcSat,dstSat,srcTyp,srcGs,dstTyp,dstGs,hops,created,recorded,queueDelay,procDelay,transDelay,propDelay";fileStatsTmp << "\n";
    fileStatsTmp.close();

    // write header for route file
    std::ofstream fileRouteTmp;
    fileRouteTmp.open(filenameRoutes);
    fileRouteTmp << "pid,type,id,lat,lon,alt";
    fileRouteTmp << "\n";
    fileRouteTmp.close();

    // open files for appending
    fileQueue.open(filenameQueue, std::ios_base::app);
    fileStats.open(filenameStats, std::ios_base::app);
    fileRoutes.open(filenameRoutes, std::ios_base::app);
}

void PacketRecorder::handleMessage(cMessage *msg) {
    error("Unsupported");
}

/** IMPLEMENTATION */
void PacketRecorder::recordPacket(inet::Packet *packet, PacketDropReason reason) {
    Enter_Method("recordPacket");
    auto routingTag = packet->getTag<routing::CstRoutingTag>();
    EV << "PacketRecorder::recordPacket" <<endl;
    msgid_t pid = packet->getId();

    routing::CstPacketType type = routingTag->getType();

    inet::units::values::B size = packet->getTotalLength();

    int srcSat = routingTag->getSrcSat();
    int dstSat = routingTag->getDstSat();

    int srcGsOrDev = routingTag->getSrcGsOrDev();
    int dstGsOrDev = routingTag->getDstGsOrDev();

    //int srcGs = routingTag->getSrcGs();
    int srcId = routingTag->getSrcId();

    //int dstGs = routingTag->getDstGs();
    int dstId = routingTag->getDstId();

    int hops = routingTag->getHops();

    double created = packet->getTag<inet::CreationTimeTag>()->getCreationTime().dbl();
    double recorded = simTime().dbl();
    EV << "created: " << created <<endl;

    // Queuing delay
    double queueDelay = 0.0;
    auto queueingTag = packet->getTag<inet::QueueingTimeTag>();
    // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes
    for (size_t i = 0; i < queueingTag->getBitTotalTimesArraySize(); i++) {
        queueDelay += queueingTag->getBitTotalTimes(i).dbl();
    }

    // Processing delay
    double procDelay = 0.0;
    auto procTag = packet->getTag<inet::ProcessingTimeTag>();
    // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes
    for (size_t i = 0; i < procTag->getBitTotalTimesArraySize(); i++) {
        procDelay += procTag->getBitTotalTimes(i).dbl();
    }

    // Transmission delay
    double transDelay = 0.0;
    auto transTag = packet->getTag<inet::TransmissionTimeTag>();
    // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes
    for (size_t i = 0; i < transTag->getBitTotalTimesArraySize(); i++) {
        transDelay += transTag->getBitTotalTimes(i).dbl();
    }

    // Propagation delay
    double propDelay = 0.0;
    auto propTag = packet->getTag<inet::PropagationTimeTag>();
    // INET 4.5: TimeTag API renamed TotalTimes -> BitTotalTimes
    for (size_t i = 0; i < propTag->getBitTotalTimesArraySize(); i++) {
        propDelay += propTag->getBitTotalTimes(i).dbl();
    }

    // write statistics
    // id
    fileStats << pid;
    // type
    if (type == routing::CstPacketType::NORMAL) {
        fileStats << "," << "N";
    } else if (type == routing::CstPacketType::CONTROL) {
        fileStats << "," << "C";
    } else {
        error("UNSUPPORTED PKTTYPE %d", type);
    }
    // reason
    fileStats << "," << reason;
    // size
    fileStats << "," << size.get();
    // src/dst
    fileStats << "," << srcSat;
    fileStats << "," << dstSat;

    if(srcGsOrDev == 0){
        fileStats << "," << "GS";
    }else if(srcGsOrDev == 1){
        fileStats << "," << "DEV";
    }else{
        error("UNSUPPORTED SRC TYPE");
    }
    fileStats << "," << srcId;

    if(dstGsOrDev == 0){
        fileStats << "," << "GS";
    }else if(dstGsOrDev == 1){
        fileStats << "," << "DEV";
    }else{
        error("UNSUPPORTED DST TYPE");
    }
    fileStats << "," << dstId;

    // hops
    fileStats << "," << hops;
    // times
    fileStats << "," << core::roundTo(created, 6);
    fileStats << "," << core::roundTo(recorded, 6);

    // delays
    fileStats << "," << core::roundTo(queueDelay, 6);
    fileStats << "," << core::roundTo(procDelay, 6);
    fileStats << "," << core::roundTo(transDelay, 6);
    //fileStats << "," << core::roundTo(0, 6);
    //fileStats << "," << core::roundTo(0, 6);
    //fileStats << "," << core::roundTo(0, 6);


    fileStats << "," << core::roundTo(propDelay, 6);
    //fileStats << "," << core::roundTo(0, 6);
    fileStats << "\n";

    // write route
    if(routingTag->getRouteArraySize() > 1){
        for (size_t i = 0; i < routingTag->getRouteArraySize(); i++) {
            auto hop = routingTag->getRoute(i);
            // id
            fileRoutes << pid;
            // type
            if (hop.type == routing::HopType::GS) {
                fileRoutes << ","
                           << "G";
            } else if (hop.type == routing::HopType::SAT) {
                fileRoutes << ","
                           << "S";
            } else if (hop.type == routing::HopType::DEV) {
                fileRoutes << ","
                           << "D";
            } else {
                error("UNSUPPORTED HOPTYPE %d", hop.type);
            }
            fileRoutes << "," << hop.id;
            fileRoutes << "," << core::roundTo(hop.lat, 2);
            fileRoutes << "," << core::roundTo(hop.lon, 2);
            fileRoutes << "," << hop.alt;

            fileRoutes << "\n";
        }
    }
}

void PacketRecorder::recordQueueSize(int satId, u_int16_t queueSize) {
    double time = core::roundTo(simTime().dbl(), 6);
    fileQueue << satId;
    fileQueue << "," << time;
    fileQueue << "," << queueSize;
    fileQueue << "\n";
}

}  // namespace statistics

/*
 * Ter_App.cc
 *
 *  Created on: Oct 22, 2025
 *      Author: root
 */


//#include "LoRa/LoRaTagInfo_m.h"
//#include "topologycontrol/TopologyControlBase.h"
//#include "LoRaApp/SimpleLoRaApp.h"
//#include "LoRaPhy/LoRaPhyPreamble_m.h"
//#include "LoRa/LoRaMac/LoRaMacFrame_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/static/StationaryMobility.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/TimeTag.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
//#include "mobility/UniformGroundMobility.h"
//#include "mobility/GroundStationMobility.h"
#include "inet/applications/base/ApplicationPacket_m.h"
//#include "KiWan/KiWanFrame_m.h"

#include "Ter_App.h"

Define_Module(Ter_App);

Ter_App::~Ter_App()
{
    cancelAndDelete(endAckTime);
    cancelAndDelete(sendUplink);
}

void Ter_App::initialize(int stage)
{
    EV << "Ter_App::initialize" <<endl;
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        cModule *containing = getContainingNode(this);
        receivedPackets = 0;

        //mlbox = check_and_cast<Transmission_Predictor*>(getSubmodule("mlBox"));
        //mlbox = check_and_cast<Transmission_Predictor*>(getSystemModule()->getSubmodule("transmissionPredictor"));

        // load model
        //mlbox->loadModel();
    }


    if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus*>(findContainingNode(
                this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        // Parameters
        timeToFirstPacket = par("timeToFirstPacket");
        timeToNextPacket  = par("timeToNextPacket");
        timeToResponse    = par("timeToResponse");
        timeToRetry       = par("timeToRetry");
        ackTimeout        = par("ackTimeout"); //acknowledgment timeout
        timer             = par("timer"); //timer used for acknowledgment implementation

        EV << "After read parameters"<<endl;
        // Messages
        endAckTime = new cMessage("acknowledgment Timeout"); //signal to notice about end of ACK time
        sendUplink = new cMessage("sendMeasurements");

        //synchronizer = new cMessage("Ask for device local time"); //signal for future use
        //joining = new cMessage("Try to reach GW"); //signal for future use
        //joiningAns = new cMessage("Can't reach GW ? .."); //signal for future use

        // Schedule first packet
        EV << "ScheduleAt: " << simTime() + timeToFirstPacket <<endl;
        scheduleAt(simTime() + timeToFirstPacket, sendUplink);
        timer = simTime() + timeToFirstPacket;

        // Schedule first packet ack timeout
        ///if (par("usingAck").boolValue())
        ///    scheduleAt(simTime() + timeToFirstPacket + ackTimeout, endAckTime);

        // Initialize  metrics
        receivedAckPackets = 0;
        sentPackets = 0;
        receivedADRCommands = 0;

        // Initialize parameters
        numberOfPacketsToSend = par("numberOfPacketsToSend");
        loRaTP = par("initialLoRaTP").doubleValue();
        loRaCF = units::values::Hz(par("initialLoRaCF").doubleValue());
        loRaSF = par("initialLoRaSF");
        loRaBW = inet::units::values::Hz(par("initialLoRaBW").doubleValue());
        loRaCR = par("initialLoRaCR");
        loRaUseHeader = par("initialUseHeader");
        pingSlot = par("initialPingSlot");
        evaluateADRinNode = par("evaluateADRinNode");

        // Metrics
        LoRa_AppPacketSent = registerSignal("LoRa_AppPacketSent");
        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");
        WATCH(receivedAck);

        // loraNodeId
        loraNodeId = getParentModule()->getIndex();//getId();

        //maxHops
        maxHops = par("maxHops").intValue();

        // Ter
        ter = check_and_cast<Ter *>(getParentModule());
        // ACHF
        ter_mob = check_and_cast<Ter_Mob *>(ter->getSubmodule("mob"));
        sat_mob = check_and_cast<Sat_Mob_NoradA *>(getSystemModule()->getSubmodule("satellite", 0)->getSubmodule("NoradModule"));

    }
}

void Ter_App::handleMessage(cMessage *msg)
{
    EV << "Ter_App::handleMessage" <<endl;
    // Either ack timer expired, or send measurement
    if (msg->isSelfMessage())
    {
        EV << "isSelfMessage()"<<endl;
        // Ack timer expired, No Ack received
        if (msg == endAckTime)
        {
            EV << "(msg == endAckTime)"<<endl;
            receivedAck = false;

            // More packets to send
            if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
            {
                // Schedule next packet
                sendUplink = new cMessage("sendMeasurements");
                scheduleAt(simTime() + timeToNextPacket - ackTimeout,
                        sendUplink);

                // Schedule next packet ack timeout
                endAckTime = new cMessage("Ack Timeout");
                scheduleAt(simTime() + timeToNextPacket, endAckTime);
                timer = timeToNextPacket + timer;
            }
        }

        // New packet to send
        if ((msg == sendUplink))
        {
            EV << "(msg == sendUplink)"<<endl;
            delete msg;

            // Send new packet
            sendUplinkPacket();
            sentPackets++;

            // More packets to send
            if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
            {
                // Schedule next packet
                sendUplink = new cMessage("sendMeasurements");
                scheduleAt(simTime() + timeToNextPacket, sendUplink);

                // Schedule next packet ack timeout
                /*if (par("usingAck").boolValue())
                {
                    endAckTime = new cMessage("Ack Timeout");
                    scheduleAt(simTime() + timeToNextPacket, endAckTime);
                    timer = timeToNextPacket + timer;
                }
                */
            }

        }
    }

}


void Ter_App::sendUplinkPacket()
{
    auto uplinkPacket = new Packet("DataFrame");
    uplinkPacket->setKind(DATA);

    auto payload = makeShared<LoRaAppPacket>();
    payload->setChunkLength(B(par("payloadSize").intValue()));

    lastSentMeasurement = rand();
    payload->setSampleMeasurement(lastSentMeasurement);

    if (evaluateADRinNode && sendNextPacketWithADRACKReq)
    {
        auto opt = payload->getOptions();
        opt.setADRACKReq(true);
        payload->setOptions(opt);
        //request->getOptions().setADRACKReq(true);
        sendNextPacketWithADRACKReq = false;
    }

    encapsulate(uplinkPacket);
    uplinkPacket->insertAtBack(payload);
    // Send packet
    EV << "Previous to uplinkPacket"<<endl;

    try{
        EV << "Ter_App::sendUplinkPacket" <<endl;
        EV << "Before call predict" <<endl;

        /*
        satellite::Sat *sat = check_and_cast<satellite::Sat *>(getSystemModule()->getSubmodule("satellite", 0));
        float latDev  = float(ter->getLatitude());
        float longDev = float(ter->getLongitude());
        double elevSat_temp = sat->getElevation(*ter);
        float elevSat = float(elevSat_temp);
        float loraTP  = float((mW(math::dBmW2mW(loRaTP)).get())/1000); // We split this value to put in the same range than dataset, since we use a Scaler this does not affect and should be in the same range.
        float loraSF  = float(loRaSF);

        // COmpute Doppler Frequency
        Hz originalFreq = loRaCF; //loraTransmission->getCenterFrequency();

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

        EV << "time= " << core::roundTo(simTime().dbl(), 6) <<endl;
        EV << "latDev=" << latDev
           << ", longDev=" << longDev
           << ", elevSat=" << elevSat
           << ", loraTP=" << loraTP
           << ", loraSF=" << loraSF
           << ", doppler=" << doppler
           << ", alt=" << alt
           << ", raan=" << raan << endl;
        std::vector<float>* myFeatures = new std::vector<float>({latDev, longDev, elevSat, loraTP, loraSF, doppler, alt, raan});
        std::vector<float> output = mlbox->predict(myFeatures);
        */

        //////auto transmissionTag = uplinkPacket->addTagIfAbsent<statistics::CstTransmissionStatisticsTag>();
        //////transmissionTag->setElevSat(elevSat_temp);

    }catch(const std::exception &NoSuchRouteException){

    }

    send(uplinkPacket, "appOut"); //lowerOut

    // Evaluate ADR
    if (evaluateADRinNode && receivedAck == false)
    {
        ADR_ACK_CNT++;
        EV << ADR_ACK_CNT << endl;
        if (ADR_ACK_CNT == ADR_ACK_LIMIT)
            sendNextPacketWithADRACKReq = true;

        if (ADR_ACK_CNT >= ADR_ACK_LIMIT + ADR_ACK_DELAY)
        {
            ADR_ACK_CNT = 0;
            increaseSFIfPossible();
            EV << "i'm working on the ADRNode " << endl;
            EV << loRaSF << endl;
            EV << loRaTP << endl;
        }
    }
    //emit(LoRa_AppPacketSent, loRaSF);
}

void Ter_App::encapsulate(Packet *packet) {
    packet->addTagIfAbsent<inet::CreationTimeTag>();
    packet->addTagIfAbsent<inet::QueueingTimeTag>();
    packet->addTagIfAbsent<inet::ProcessingTimeTag>();
    packet->addTagIfAbsent<inet::TransmissionTimeTag>();
    packet->addTagIfAbsent<inet::PropagationTimeTag>();


    auto transmissionTag = packet->addTagIfAbsent<statistics::CstTransmissionStatisticsTag>();
    transmissionTag->setLatDev(ter->getLatitude());
    transmissionTag->setLongDev(ter->getLongitude());
    //transmissionTag->setSatId(satIndex);
    //transmissionTag->setElevSat(sat->getElevation(*ter));
    //transmissionTag->setDoppler(dopplerShift);

    auto routingTag = packet->addTagIfAbsent<routing::CstRoutingTag>();
    routingTag->setType(routing::CstPacketType::NORMAL);

    routingTag->setSrcId(loraNodeId);
    routingTag->setDstId(-1);
    routingTag->setMaxHops(maxHops);
    routingTag->setSrcGsOrDev(1); // Set the source to device
    routingTag->setDstGsOrDev(0); //  Set the dest to GS or Dev

    EV << "loraNodeId: " << loraNodeId <<endl;
    EV << "loraNodeId: " << routingTag->getMaxHops() <<endl;

    // /*
    auto hop = routing::Hop();
    hop.type = routing::HopType::DEV;
    hop.id   = loraNodeId;


    hop.lat = ter->getLatitude();
    hop.lon = ter->getLongitude();
    hop.alt = (int)round(ter->getAltitude());

    routingTag->setIsLoraNode(1); // Set to LoraNode
    routingTag->appendRoute(hop);

}

void Ter_App::decapsulate(Packet *packet) {
    //numReceived++;
    //receivedBytes += packet->getTotalLength();
    //emit(packetReceivedSignal, packet);
}

void Ter_App::increaseSFIfPossible(){
    if (loRaSF < 12)
        loRaSF++;
}

bool Ter_App::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

std::pair<std::pair<int, int>, std::pair<int, int>> Ter_App::recalculateRoute(){
}

void Ter_App::finish(){

}

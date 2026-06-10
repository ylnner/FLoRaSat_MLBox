/*
 * ConstellationCreator.cc
 *
 *  Created on: Oct 21, 2025
 *      Author: root
 */


#include "ConstellationCreator.h"
//using namespace Global;

namespace Global {
namespace constellation{

Define_Module(ConstellationCreator);

ConstellationCreator::ConstellationCreator() : planeCount(0),
                                               inclination(0.0),
                                               altitude(0),
                                               interPlaneSpacing(0),
                                               baseYear(0),
                                               baseDay(0.0),
                                               epochYear(0),
                                               epochDay(0.0),
                                               eccentricity(0.0),
                                               raanSpread(0),
                                               satsPerPlane(0),
                                               satCount(0) {
}

void ConstellationCreator::initialize() {
    bool enabled = par("enabled");
    EV << "ConstellationCreator::initialize()" << endl;
    EV << "ConstellationCreator enabled: " << enabled << endl;
    
    // Skip constellation creation if disabled (e.g., when using TLE files)
    if (!enabled) {
        EV_INFO << "ConstellationCreator is disabled - skipping Walker constellation generation. "
                << "Satellites will use their configured NoradModule (e.g., Sat_Mob_NoradTLE with TLE data)." << endl;
        return;
    }
    
    planeCount          = par("planeCount");
    inclination         = par("inclination");
    altitude            = par("altitude");
    interPlaneSpacing   = par("interPlaneSpacing");
    baseYear            = par("baseYear");
    baseDay             = par("baseDay");
    epochYear           = par("epochYear");
    epochDay            = par("epochDay");
    eccentricity        = par("eccentricity");
    raanSpread          = par("raanSpread");

    // get satellite number and compute sats_per_plane
    satCount = getParentModule()->getSubmoduleVectorSize("satellite");
    satsPerPlane = satCount / planeCount;

    // Validate DATA
    VALIDATE(interPlaneSpacing >= 0 && interPlaneSpacing < planeCount);
    VALIDATE(altitude > 0);
    VALIDATE(raanSpread == 180 || raanSpread == 360);
    VALIDATE(satCount > 0);
    VALIDATE(planeCount > 0);
    VALIDATE(satsPerPlane > 0);
    VALIDATE(satsPerPlane * planeCount == satCount);

    createSatellites();
}

void ConstellationCreator::createSatellites() {
    double raanDelta = (double)raanSpread / planeCount;           // ΔΩ = 2𝜋/𝑃 in [0,2𝜋]
    double phaseDifference = 360.0 / satsPerPlane;                // ΔΦ = 2𝜋/Q in [0,2𝜋]
    double phaseOffset = (360.0 * interPlaneSpacing) / satCount;  // Δ𝑓 = 2𝜋𝐹/𝑃𝑄 in [0,2𝜋[
    VALIDATE(raanDelta >= 0 && raanDelta <= 360.0);
    VALIDATE(phaseDifference >= 0 && phaseDifference <= 360.0);
    VALIDATE(phaseOffset >= 0 && phaseOffset < 360.0);
    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        // create plane satellites
        double raan = raanDelta * plane;
        VALIDATE(raan >= 0 && raan < 360.0);
        for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
            int index = planeSat + plane * satsPerPlane;
            double meanAnomaly = std::fmod(plane * phaseOffset + planeSat * phaseDifference, 360.0);
            VALIDATE(meanAnomaly >= 0 && meanAnomaly < 360.0);
            createSatellite(index, raan, meanAnomaly, plane);
        }
    }
}

void ConstellationCreator::createSatellite(int index, double raan, double meanAnomaly, int plane) {
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): parent is nullptr");
    }
    //cModule *sat = parent->getSubmodule("loRaGW", index);
    cModule *sat = parent->getSubmodule("satellite", index);
    if (sat == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): sat(%d) is nullptr", index);
    }
    cModule *noradBaseModule = sat->getSubmodule("NoradModule");
    if (noradBaseModule == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): noradModule of sat(%d) is nullptr", index);
    }

    INorad *oldNoradModule = dynamic_cast<INorad *>(noradBaseModule);
    if (oldNoradModule == nullptr) {
        EV_WARN << "ConstellationCreator: skipping satellite " << index
                << " because NoradModule is not INorad" << endl;
        return;
    }

    const char *satName = oldNoradModule->par("satName").stringValue();

    oldNoradModule->deleteModule();

    //cModule *noradModule = cModuleType::get("flora.mobility.NoradA")->create("NoradModule", sat);
    cModule *noradModule = cModuleType::get("Nodes._20_Satellite._60_Mobility.Sat_Mob_NoradA")->create("NoradModule", sat);

    if (noradModule == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): Cannot create \"Nodes._20_Satellite._60_Mobility.Sat_Net_NoradA\".");
    }
    noradModule->par("satIndex").setIntValue(index);
    noradModule->par("baseYear").setIntValue(baseYear);
    noradModule->par("baseDay").setDoubleValue(baseDay);
    noradModule->par("satName").setStringValue(satName);
    noradModule->par("planes").setIntValue(planeCount);
    noradModule->par("satPerPlane").setIntValue(satsPerPlane);
    noradModule->par("epochYear").setIntValue(epochYear);
    noradModule->par("epochDay").setDoubleValue(epochDay);
    noradModule->par("eccentricity").setDoubleValue(eccentricity);
    noradModule->par("inclination").setDoubleValue(inclination);
    noradModule->par("altitude").setDoubleValue(altitude);
    noradModule->par("raan").setDoubleValue(raan);
    noradModule->par("meanAnomaly").setDoubleValue(meanAnomaly);

    noradModule->finalizeParameters();
    noradModule->callInitialize();
    INorad *norad = check_and_cast<INorad *>(noradModule);
    if (norad == nullptr) {
        error("Error in ConstellationCreator::CreateSatellite(): Cannot find casted created module NoradModule.");
    }
    norad->initializeMobility(simTime());
}

} // Constellation
} // Global

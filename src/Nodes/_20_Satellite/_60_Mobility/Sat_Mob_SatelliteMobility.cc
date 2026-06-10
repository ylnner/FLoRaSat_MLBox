#include "omnetpp/csimulation.h"
#include "Sat_Mob_SatelliteMobility.h"


namespace mobility {

Define_Module(Sat_Mob_SatelliteMobility);

Sat_Mob_SatelliteMobility::Sat_Mob_SatelliteMobility() {
    mapX = 0;
    mapY = 0;
}

void Sat_Mob_SatelliteMobility::initialize(int stage) {
    if (stage == 0) { // INITSTAGE_LOCAL
        MobilityBase::initialize(stage);

        // NoradModule is optional; fall back gracefully if it is missing.
        cModule *noradSubmodule = getParentModule() ? getParentModule()->getSubmodule("NoradModule") : nullptr;
        norad = dynamic_cast<INorad *>(noradSubmodule);
        if (noradSubmodule != nullptr && norad == nullptr) {
            throw cRuntimeError("NoradModule exists but does not implement Global.Mobility.INorad");
        }
        if (norad == nullptr) {
            EV_WARN << "Sat_Mob_SatelliteMobility: NoradModule not found; mobility will remain static." << endl;
        }

        WATCH(currentPosition);
        WATCH(lastPosition);
        WATCH(currentOrientation);
        WATCH(lastOrientation);

        // Determine map size (used for lon/lat -> canvas projection).
        // 1) Prefer explicit parameters if present (works in both Cmdenv and Qtenv).
        // 2) Optionally fall back to the parent-of-parent display string "bgb" tag (Qtenv only).
        mapX = (hasPar("mapX") ? (int)par("mapX") : 0);
        mapY = (hasPar("mapY") ? (int)par("mapY") : 0);

        // Fall back to display string only if we are running with a GUI and parameters are not set.
        if ((mapX <= 0 || mapY <= 0) && omnetpp::cSimulation::getActiveEnvir()->isGUI()) {
            const char *bx = nullptr;
            const char *by = nullptr;
            try {
                if (getParentModule() && getParentModule()->getParentModule()) {
                    auto &ds = getParentModule()->getParentModule()->getDisplayString();
                    bx = ds.getTagArg("bgb", 0);
                    by = ds.getTagArg("bgb", 1);
                }
                if (mapX <= 0)
                    mapX = (bx && *bx) ? std::atoi(bx) : 0;
                if (mapY <= 0)
                    mapY = (by && *by) ? std::atoi(by) : 0;
            }
            catch (...) {
                // Never crash due to GUI/display-string issues.
                mapX = (mapX > 0) ? mapX : 0;
                mapY = (mapY > 0) ? mapY : 0;
            }
        }
        // If we have no NORAD provider, keep the (0,0,0) position and do not schedule NORAD-driven init.
        // If we have a NORAD provider, initialize it now (avoid relying on INET InitStageRegistry constants).
        if (norad != nullptr) {
            norad->initializeMobility(SimTime::ZERO);
            // Update position immediately 
            updatePosition(SimTime::ZERO);
        }
    }
}

void Sat_Mob_SatelliteMobility::handleSelfMessage(cMessage* message) {
    throw new cRuntimeError("Error in SatMobility::handleSelfMessage: this module should not receive messages.");
}

double Sat_Mob_SatelliteMobility::getDistance(const double& refLatitude, const double& refLongitude,
                                      const double& refAltitude) const {
    return norad ? norad->getDistance(refLatitude, refLongitude, refAltitude) : 0.0;
}
double Sat_Mob_SatelliteMobility::getLongitude() const {
    return norad ? norad->getLongitude() : 0.0;
}

double Sat_Mob_SatelliteMobility::getLatitude() const {
    return norad ? norad->getLatitude() : 0.0;
}

double Sat_Mob_SatelliteMobility::getAltitude() const {
    return norad ? norad->getAltitude() : 0.0;
}

void Sat_Mob_SatelliteMobility::updatePosition(SimTime currentTime) {
    if (norad == nullptr) {
        // No NORAD provider: keep static state.
        lastUpdate = currentTime;
        return;
    }

    norad->updateTime(currentTime);
    lastPosition.x = currentPosition.x;
    lastPosition.y = currentPosition.y;
    lastPosition.z = currentPosition.z;
    currentPosition.x = getXCanvas(norad->getLongitude());  // x canvas position, longitude projection
    currentPosition.y = getYCanvas(norad->getLatitude());   // y canvas position, latitude projection
    currentPosition.z = norad->getAltitude();               // real satellite altitude in km

    velocity = (currentPosition - lastPosition) / (currentTime - lastUpdate).dbl();
    orient();

    raiseErrorIfOutside();
    emitMobilityStateChangedSignal();
}

void Sat_Mob_SatelliteMobility::orient() {
    if (faceForward) {
        // determine orientation based on direction
        if (velocity != inet::Coord::ZERO) {
            inet::Coord direction = velocity;
            direction.normalize();
            auto alpha = inet::rad(atan2(direction.y, direction.x));
            auto beta = inet::rad(-asin(direction.z));
            auto gamma = inet::rad(0.0);
            lastOrientation = inet::Quaternion(inet::EulerAngles(alpha, beta, gamma));
        }
    }
}

double Sat_Mob_SatelliteMobility::getXCanvas(double lon) const {
    // double x = mapX * lon / 360 + (mapX / 2);
    // return static_cast<int>(x) % static_cast<int>(mapX);
    if (mapX <= 0)
        return lon;
    return ((mapX * lon) / 360) + (mapX / 2);
}

double Sat_Mob_SatelliteMobility::getYCanvas(double lat) const {
    if (mapY <= 0)
        return lat;
    return ((-mapY * lat) / 180) + (mapY / 2);
}

const inet::Coord& Sat_Mob_SatelliteMobility::getCurrentPosition() {
    // updatePosition(simTime());
    return currentPosition;
}

const inet::Coord& Sat_Mob_SatelliteMobility::getCurrentVelocity() {
    // updatePosition(simTime());
    return velocity;
}

const inet::Quaternion& Sat_Mob_SatelliteMobility::getCurrentAngularPosition() {
    // updatePosition(simTime());
    return currentOrientation;
}

const inet::Quaternion& Sat_Mob_SatelliteMobility::getCurrentAngularVelocity() {
    // updatePosition(simTime());
    return angularVelocity;
}
}  // namespace mobility



/*

namespace mobility{
Define_Module(Sat_Mob_SatelliteMobility);

Sat_Mob_SatelliteMobility::Sat_Mob_SatelliteMobility() {
    noradModule = nullptr;
    mapX = 0;
    mapY = 0;
    transmitPower = 0.0;
}

void Sat_Mob_SatelliteMobility::initialize(int stage) {
    // noradModule must be initialized before LineSegmentsMobilityBase calling setTargetPosition() in its initialization at stage 1
    if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT) {
        noradModule->initializeMobility(nextChange);
    }
    LineSegmentsMobilityBase::initialize(stage);
    noradModule = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
    if (noradModule == nullptr) {
        error("Error in SatelliteMobility::initializeMobility(): Cannot find module Norad.");
    }

    // get current time as an integral value holding the num of secs since 00:00, Jan 1 1970 UTC
    // std::time_t timestamp =  1577904000;              // 01-01-2020 18:40:00 UTC
    // std::tm* currentTime = std::gmtime(&timestamp);   // convert timestamp into structure holding a calendar date and time
    // noradModule->setJulian(currentTime);

    mapX = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
    mapY = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));

    EV << "initializing SatelliteMobility stage " << stage << endl;
    WATCH(lastPosition);

    effectiveSlant = par("effectiveSlant");
    displaySpanArea = par("displaySpanArea");
    if (stage == INITSTAGE_LAST && displaySpanArea) {
        refreshArea = new cMessage("refreshArea");
        polygon = new cPolygonFigure("polygon");
        polygon->setNumPoints(nPoints);
        polygon->setSmooth(true);
        polygon->setFilled(true);
        polygon->setLineWidth(1);
        polygon->setFillOpacity(0.15);
        polygon->setLineColor(cFigure::RED);
        polygon->setFillColor(cFigure::RED);
        setAllPoints();
        networkCanvas = getParentModule()->getParentModule()->getCanvas();
        networkCanvas->addFigure(polygon);

        EV << "satellite position is lat= " << getLatitude() << "; lon= " << getLongitude() << endl;
    }
}

void Sat_Mob_SatelliteMobility::initializePosition() {
    nextChange = simTime();
    LineSegmentsMobilityBase::initializePosition();
}

bool Sat_Mob_SatelliteMobility::isOnSameOrbitalPlane(double raan2, double inclination2) {
    if (Sat_Mob_NoradA* noradAModule = dynamic_cast<Sat_Mob_NoradA*>(noradModule)) {
        double raan = noradAModule->getRaan();
        double inclination = noradAModule->getInclination();
        if ((inclination == inclination2) && (raan == raan2)) {
            return true;
        }
    }
    return false;
}

double Sat_Mob_SatelliteMobility::getAltitude() const {
    return noradModule->getAltitude();
}

double Sat_Mob_SatelliteMobility::getElevation(const double& refLatitude, const double& refLongitude,
                                       const double& refAltitude) const {
    return noradModule->getElevation(refLatitude, refLongitude, refAltitude);
}

double Sat_Mob_SatelliteMobility::getAzimuth(const double& refLatitude, const double& refLongitude,
                                     const double& refAltitude) const {
    return noradModule->getAzimuth(refLatitude, refLongitude, refAltitude);
}

double Sat_Mob_SatelliteMobility::getDistance(const double& refLatitude, const double& refLongitude,
                                      const double& refAltitude) const {
    return noradModule->getDistance(refLatitude, refLongitude, refAltitude);
}

double Sat_Mob_SatelliteMobility::getLongitude() const {
    return noradModule->getLongitude();
}

double Sat_Mob_SatelliteMobility::getLatitude() const {
    return noradModule->getLatitude();
}

void Sat_Mob_SatelliteMobility::setTargetPosition() {
    nextChange += updateInterval.dbl();
    noradModule->updateTime(nextChange);
    lastPosition.x = getXCanvas(getLongitude());  // x canvas position, longitude projection
    lastPosition.y = getYCanvas(getLatitude());   // y canvas position, latitude projection
    lastPosition.z = getAltitude();               // real satellite altitude in km
    targetPosition.x = lastPosition.x;
    targetPosition.y = lastPosition.y;
    targetPosition.z = lastPosition.z;
}

double Sat_Mob_SatelliteMobility::getXCanvas(double lon) const {
    double x = mapX * lon / 360 + (mapX / 2);
    return static_cast<int>(x) % static_cast<int>(mapX);
}

double Sat_Mob_SatelliteMobility::getYCanvas(double lat) const {
    return ((-mapY * lat) / 180) + (mapY / 2);
}

void Sat_Mob_SatelliteMobility::move() {
    LineSegmentsMobilityBase::move();
    raiseErrorIfOutside();
}

void Sat_Mob_SatelliteMobility::handleSelfMessage(cMessage* msg) {
    moveAndUpdate();
    scheduleUpdate();
    if (displaySpanArea) {
        polygon->setVisible(false);
        removeAllPoints();
        setAllPoints();
        polygon->setVisible(true);
    }
}

void Sat_Mob_SatelliteMobility::removeAllPoints() {
    for (int i = 0; i < nPoints; i++)
        polygon->removePoint(0);
    polygon->setNumPoints(nPoints);
}

void Sat_Mob_SatelliteMobility::setAllPoints() {
    double alt = lastPosition.z;  // altitude
    double r = XKMPER_WGS72;      // earth radius
    double r2 = r * r;
    double alt2 = alt * alt;
    double hr = alt * r;
    double slant = sqrt(alt2 + 2 * hr);  // slant range
    double alpha = atan(slant / r);      // view angle
    double b, xi, yi, xCanvas, yCanvas;

    // satellite projection on Earth
    cEcef* P = new cEcef(getLatitude(), getLongitude(), r);
    Coord* Px = new Coord(P->getX(), P->getY(), P->getZ());

    // null island on Earth
    cEcef* O = new cEcef(0, 0, r);
    Coord* Ox = new Coord(O->getX(), O->getY(), O->getZ());

    // get cross and dot products
    Coord cross = *Ox % *Px;
    double angle = Px->angle(*Ox);
    cross.normalize();

    // get rotation quaternion
    Quaternion* q = new Quaternion(cross, angle);

    delete P;
    delete O;
    delete Px;
    delete Ox;

    // effective area considering max range given lora parameters
    double slantAngle = alpha;
    if (effectiveSlant > 0) {
        double x = 2 * r2 + 2 * hr;
        double z = (x + alt2 - effectiveSlant * effectiveSlant) / x;
        double beta = acos(z);
        slantAngle = beta;
    }

    // solve span area perimeter points centered around O, then apply rotation
    for (int i = 0; i < nPoints - 1; i++) {
        b = i * TWOPI / nPoints;
        xi = slantAngle * cos(b) / RADS_PER_DEG;  // longitude
        yi = slantAngle * sin(b) / RADS_PER_DEG;  // latitude

        cEcef* pointEcef = new cEcef(yi, xi, r);
        Coord* pointCoord = new Coord(pointEcef->getX(), pointEcef->getY(), pointEcef->getZ());

        Coord rotatedCoord = q->rotate(*pointCoord);
        cEcef* rotatedEcef = new cEcef(rotatedCoord.getX(), rotatedCoord.getY(), rotatedCoord.getZ(), 0);

        xCanvas = getXCanvas(rotatedEcef->getLongitude());
        yCanvas = getYCanvas(rotatedEcef->getLatitude());
        polygon->setPoint(i, cFigure::Point(xCanvas, yCanvas));

        delete pointEcef;
        delete pointCoord;
        delete rotatedEcef;

        // EV << "point lon: " << rotatedEcef->getLongitude() << "; point lat: " << rotatedEcef->getLatitude()<< endl;
    }

    polygon->setPoint(nPoints - 1, polygon->getPoint(0));
}

void Sat_Mob_SatelliteMobility::fixIfHostGetsOutside() {
    raiseErrorIfOutside();
}
}  // namespace mobility

*/

/*
 * DiscoRoute.cc
 *
 * Created on: Jun 09, 2023
 *     Author: Robin Ohs
 */

#include "MinHopCount.h"

namespace routing {
namespace core {

double fpmod(double dividend, double divisor) {
    auto res = fmod(dividend, divisor);
    return res < 0 ? divisor + res : res;
}

double roundCom(double x) {
    bool neg = x < 0;
    double val = floor(abs(x) + 0.5);
    return neg ? -1 * val : val;
}

MinInterPlaneHopsRes minInterplaneHops(double raanSrc, double raanDst, double raanDiff) {
    double l0_e = fpmod(raanDst - raanSrc);
    double l0_w = fpmod(360.0 - l0_e);
    int west = roundCom(l0_w / raanDiff);
    int east = roundCom(l0_e / raanDiff);
    // std::cout << "InterPlaneHops:" << endl;
    // std::cout << "  RAAN-Src: " << raanSrc << endl;
    // std::cout << "  RAAN-Dst: " << raanDst << endl;
    // std::cout << "  raanDiff: " << raanDiff << endl;
    // std::cout << "  l0_e: " << l0_e << endl;
    // std::cout << "  l0_w: " << l0_w << endl;
    // std::cout << "  hops east: " << east << endl;
    // std::cout << "  hops west: " << west << endl;
    // std::cout << endl;
    return MinInterPlaneHopsRes{west, east};
}

MinIntraPlaneHopsRes minIntraPlaneHops(MinInterPlaneHopsRes interPlaneHops, double uSrc, double uDst, double phaseDiff, double f) {
    double phaseAngleDiff = uDst - uSrc;
    double phasingAngleByEastInterPlaneHops = interPlaneHops.east * f;
    double phasingAngleByWestInterPlaneHops = interPlaneHops.west * f;

    double delta_U_East = fpmod(phaseAngleDiff - phasingAngleByEastInterPlaneHops);
    double delta_U_West = fpmod(phaseAngleDiff + phasingAngleByWestInterPlaneHops);

    int hV_UpWest = roundCom(delta_U_West / phaseDiff);
    int hV_UpEast = roundCom(delta_U_East / phaseDiff);

    int hV_DownWest = roundCom((360.0 - delta_U_West) / phaseDiff);
    int hV_DownEast = roundCom((360.0 - delta_U_East) / phaseDiff);

    // std::cout << "IntraPlaneHops:" << endl;
    // std::cout << "  PhasingAngle-Src (uSrc): " << uSrc << endl;
    // std::cout << "  PhasingAngle-Dst (uDst): " << uDst << endl;
    // std::cout << "  PhasingAngle-Diff: " << phaseAngleDiff << endl;
    // std::cout << "  PhasingAngle-FPmodDiff: " << fpmod(phaseAngleDiff) << endl;
    // std::cout << "    -> Jumps Up: " << roundCom(fpmod(phaseAngleDiff - phasingAngleByEastInterPlaneHops) / phaseDiff) << endl;
    // std::cout << "    -> Jumps Down: " << roundCom((360.0 - fpmod(phaseAngleDiff + phasingAngleByWestInterPlaneHops)) / phaseDiff) << endl;
    // std::cout << "  PhasingAngle-East-Jumps: " << phasingAngleByEastInterPlaneHops << endl;
    // std::cout << "  PhasingAngle-West-Jumps: " << phasingAngleByWestInterPlaneHops << endl;
    // std::cout << endl;
    // std::cout << "  delta_U_East: " << delta_U_East << "(" << phaseAngleDiff << "-" << phasingAngleByEastInterPlaneHops << ")" << endl;
    // std::cout << "  delta_U_West: " << delta_U_West << "(" << phaseAngleDiff << "+" << phasingAngleByWestInterPlaneHops << ")" << endl;
    // std::cout << endl;
    // std::cout << "  hV_UpWest: " << hV_UpWest << endl;
    // std::cout << "  hV_UpEast: " << hV_UpEast << endl;
    // std::cout << "  hV_DownWest: " << hV_DownWest << endl;
    // std::cout << "  hV_DownEast: " << hV_DownEast << endl;
    // std::cout << endl;
    return MinIntraPlaneHopsRes{hV_UpWest, hV_UpEast, hV_DownWest, hV_DownEast};
}

MinHopsRes minHops(double uSrc, double raanSrc, double uDst, double raanDst, double raanDiff, double phaseDiff, double f) {
    MinInterPlaneHopsRes interPlaneRes = minInterplaneHops(raanSrc, raanDst, raanDiff);
    MinIntraPlaneHopsRes intraPlaneRes = minIntraPlaneHops(interPlaneRes, uSrc, uDst, phaseDiff, f);
    int hopsWestUp = interPlaneRes.west + intraPlaneRes.upLeft;
    int hopsWestDown = interPlaneRes.west + intraPlaneRes.downLeft;
    int hopsEastUp = interPlaneRes.east + intraPlaneRes.upRight;
    int hopsEastDown = interPlaneRes.east + intraPlaneRes.downRight;

    SendDirection dir = LEFT_UP;
    int hops = hopsWestUp;
    // check west down
    if (hopsWestDown < hops) {
        dir = LEFT_DOWN;
        hops = hopsWestDown;
    }
    // check east up
    if (hopsEastUp < hops) {
        dir = RIGHT_UP;
        hops = hopsEastUp;
    }
    // check east down
    if (hopsEastDown < hops) {
        dir = RIGHT_DOWN;
        hops = hopsEastDown;
    }

    int hHorizontal;
    int hVertical;
    // std::cout << "Min Hops:" << endl;
    switch (dir) {
        case RIGHT_UP:
            hHorizontal = interPlaneRes.east;
            hVertical = intraPlaneRes.upRight;
            // std::cout << "  Dir: RightUp" << endl;
            break;
        case RIGHT_DOWN:
            hHorizontal = interPlaneRes.east;
            hVertical = intraPlaneRes.downRight;
            // std::cout << "  Dir: RightDown" << endl;
            break;
        case LEFT_UP:
            hHorizontal = interPlaneRes.west;
            hVertical = intraPlaneRes.upLeft;
            // std::cout << "  Dir: LeftUp" << endl;
            break;
        case LEFT_DOWN:
            hHorizontal = interPlaneRes.west;
            hVertical = intraPlaneRes.downLeft;
            // std::cout << "  Dir: LeftDown" << endl;
            break;
        default:
            throw new cRuntimeError("Error in MinHopCount::minHops: Unhandled SendDirection.");
            break;
    }
    // std::cout << "  West-Up: " << hopsWestUp << endl;
    // std::cout << "  West-Down: " << hopsWestDown << endl;
    // std::cout << "  East-Up: " << hopsEastUp << endl;
    // std::cout << "  East-Down: " << hopsEastDown << endl;
    // std::cout << endl;
    return MinHopsRes{interPlaneRes, intraPlaneRes, hopsWestUp, hopsWestDown, hopsEastUp, hopsEastDown, hops, dir, hHorizontal, hVertical};
}

}  // namespace core
}  // namespace routing

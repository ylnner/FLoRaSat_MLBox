/*
 * FailedLink.h
 *
 *  Created on: Jan 20, 2026
 *      Author: ylnner
 */

#ifndef NODES__20_SATELLITE__30_NETWORK_ROUTING_DDRA_FAILEDLINK_H_
#define NODES__20_SATELLITE__30_NETWORK_ROUTING_DDRA_FAILEDLINK_H_



namespace routing {

struct FailedLink {
    int srcSat;
    int dstSat;
};

FailedLink createFailedLink(int srcSat, int dstSat);

}  // namespace routing



#endif /* NODES__20_SATELLITE__30_NETWORK_ROUTING_DDRA_FAILEDLINK_H_ */

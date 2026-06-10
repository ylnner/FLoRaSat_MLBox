/*
 * NoSuchRouteException.h
 *
 * Created on: Jun 23, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_NOSUCHROUTEEXCEPTION_H_
#define __FLORA_ROUTING_CORE_NOSUCHROUTEEXCEPTION_H_

#include <exception>
#include <sstream>

namespace routing {
namespace core {

class NoSuchRouteException : public std::exception {
   public:
    NoSuchRouteException(int dst) : dst(dst){};
    const std::string msg() const {
        std::ostringstream ss;
        ss << "NoSuchRoute: No route to " << dst << " found!";
        return ss.str();
    };
    const int getSrc() const { return src; };
    const int getDst() const { return dst; };

   protected:
    int src;
    int dst;
};

}  // namespace core
}  // namespace routing

#endif  // __FLORA_ROUTING_CORE_NOSUCHROUTEEXCEPTION_H_

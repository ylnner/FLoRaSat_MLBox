/*
 * UnroutableException.h
 *
 * Created on: Jun 23, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_UNROUTABLEEXCEPTION_H_
#define __FLORA_ROUTING_CORE_UNROUTABLEEXCEPTION_H_

#include <exception>
#include <sstream>

namespace routing {
namespace core {

class UnroutableException : public std::exception {
   public:
    UnroutableException(int src, int dst) : src(src), dst(dst){};
    const std::string msg() const {
        std::ostringstream ss;
        ss << "Unroutable: No route between " << src;
        ss << "and " << dst << " found!";
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

#endif  // __FLORA_ROUTING_CORE_UNROUTABLEEXCEPTION_H_

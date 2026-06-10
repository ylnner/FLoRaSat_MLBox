#pragma once

#include <cstdint>

#include <omnetpp.h>

namespace core {

[[nodiscard]] int randomNumber(omnetpp::cModule *module, int start, int end, int excluded);

[[nodiscard]] int64_t getMillisSinceEpoch() noexcept;

[[nodiscard]] omnetpp::simtime_t getSimtimeSinceStartMillis(int64_t startTimestampMicros);

[[nodiscard]] double roundTo(double value, int decimals = 0) noexcept;

#define VALIDATE(expr)                                                                                                                \
	((void)((expr) ? 0 : (throw omnetpp::cRuntimeError("VALIDATE: Condition '%s' does not hold in function '%s' at %s:%d", #expr, \
															 __FUNCTION__, __FILE__, __LINE__),                                       \
									  0)))

#define VALIDATE_SET(expr)                                                                                                            \
	((void)(((expr) != -1) ? 0                                                                                                        \
							 : (throw omnetpp::cRuntimeError(                                                                        \
									"VALIDATE_IS_SET: Condition '%s' does not hold in function '%s' at %s:%d", #expr, __FUNCTION__,  \
									__FILE__, __LINE__),                                                                              \
								 0)))

}  // namespace core

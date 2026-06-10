#pragma once
namespace core {

struct Constants final {
    Constants() = delete;

    static constexpr const char *const ISL_CHANNEL_NAME = "IslChannel";

    static constexpr const char *const ISL_UP_NAME = "up";
    static constexpr const char *const ISL_DOWN_NAME = "down";
    static constexpr const char *const ISL_LEFT_NAME = "left";
    static constexpr const char *const ISL_RIGHT_NAME = "right";
    static constexpr const char *const SAT_GROUNDLINK_NAME = "groundLink";

    static constexpr const char *const ISL_UP_NAME_IN = "upIn";
    static constexpr const char *const ISL_DOWN_NAME_IN = "downIn";
    static constexpr const char *const ISL_LEFT_NAME_IN = "leftIn";
    static constexpr const char *const ISL_RIGHT_NAME_IN = "rightIn";
    static constexpr const char *const SAT_GROUNDLINK_NAME_IN = "groundLinkIn";

    static constexpr const char *const ISL_UP_NAME_OUT = "upOut";
    static constexpr const char *const ISL_DOWN_NAME_OUT = "downOut";
    static constexpr const char *const ISL_LEFT_NAME_OUT = "leftOut";
    static constexpr const char *const ISL_RIGHT_NAME_OUT = "rightOut";
    static constexpr const char *const SAT_GROUNDLINK_NAME_OUT = "groundLinkOut";

    static constexpr const char *const GS_SATLINK_NAME = "satelliteLink";

    static constexpr const char *const ISL_STATE_WORKING = "WORKING";
    static constexpr const char *const ISL_STATE_DISABLED = "DISABLED";

    static constexpr const char *const WALKERTYPE_DELTA = "DELTA";
    static constexpr const char *const WALKERTYPE_STAR = "STAR";
};

}  // namespace core

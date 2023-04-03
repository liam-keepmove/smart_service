#include "device_mqtt.hpp"
#include <fmt/core.h>

namespace robot_device {

json camera_mqtt::detect_bolt(const json& args) {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

json camera_mqtt::detect_meter(const json& args) {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

json camera_mqtt::detect_pull_rod(const json& args) {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

json camera_mqtt::shot() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

json action_body_mqtt::direct_speed_move(const json& args) {
    status = RUN;
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

json action_body_mqtt::location_speed_move(const json& args) {
    status = RUN;
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

void action_body_mqtt::stop() {
    status = STOP;
    return;
}

json action_body_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

json ptz_mqtt::set_xyz(const json& args) {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
}

json ptz_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

json lamp_mqtt::set_light(const json& args) {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    result["args"] = args;
    return result;
};

json lamp_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

}; // namespace robot_device
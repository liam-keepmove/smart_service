#include "device_mqtt.hpp"
#include "misc.hpp"
#include <fmt/core.h>
#include <thread>
using fmt::print;
using fmt::println;

static const char* robot_ctrl_move_topic = "/Robot/CtrlMove/500d28757093ca040eb09711";
static const char* robot_ctrl_other_topic = "/Robot/CtrlOther/500d28757093ca040eb09711";
static const char* pad_ctrl_topic = "/Pad/Ctrl/500d28757093ca040eb09711";
static const char* pantilt_ctrl_topic = "/PanTilt/Ctrl/500d28757093ca040eb09711";
static const char* robot_position_topic = "/Robot/Position/500d28757093ca040eb09711";
extern mosquitto* mosq;

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

action_body_mqtt::action_body_mqtt() {
}

action_body_mqtt::~action_body_mqtt() {
}

//{"action":0,"arg":null}
json action_body_mqtt::stop_move(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 0;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":1,"arg":0.5}
json action_body_mqtt::speed_front_move(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":2,"arg":0.5}
json action_body_mqtt::speed_back_move(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

// 位置模式,到达位置后才返回,结果包含到达的位置 {"action":3,"arg":{"speed":0.5,"position":1500}}
json action_body_mqtt::location_speed_move(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    int current_location = 0;
    const int target_location = command["arg"]["position"].template get<int>();
    auto subscribe_callback = [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
        int& current_location = *(int*)obj;
        json result = json::parse((char*)message->payload);
        result["position"].get_to(current_location);
        return 0;
    };
    while (status == RUNING) {
        if (current_location < (target_location - 20) || current_location > (target_location + 20)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            mosquitto_subscribe_callback(subscribe_callback, &current_location, robot_position_topic, 2, "127.0.0.1", 1883, "get_position", 10, true, "admin", "abcd1234", nullptr, nullptr);
        } else
            break;
    }
    status = STOPPED;
    return json::parse(fmt::format("{{\"position\": {}}}", current_location));
}

//{"action":4,"arg":null}
json action_body_mqtt::to_charge(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 4;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":5,"arg":null}
json action_body_mqtt::motor_reset(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 5;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":6,"arg":null}
json action_body_mqtt::restart(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 6;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":7,"arg":1500}
json action_body_mqtt::set_current_location(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 7;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":8,"arg":0}
json action_body_mqtt::set_ultrasonic_switch(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 8;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":9,"arg":null}
json action_body_mqtt::poweroff(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 9;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":1,"arg":50}
json action_body_mqtt::set_front_light(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":2,"arg":50}
json action_body_mqtt::set_back_light(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":3,"arg":10}
json action_body_mqtt::set_volume(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":1,"arg":45}
json action_body_mqtt::set_left_servo(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

//{"action":2,"arg":45}
json action_body_mqtt::set_right_servo(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    status = STOPPED;
    return "";
}

void action_body_mqtt::stop() {
    request = REQ_STOP;
    while (status != STOPPED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        request = REQ_STOP;
    }
    return;
}

json action_body_mqtt::get_status() {
    json result;
    return result;
}

//{"action":1,"arg":{"yaw": 180,"pitch": 180,"roll": 180}}
json ptz_mqtt::set_xyz(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, pantilt_ctrl_topic, payload);
    mosquitto_publish(mosq, nullptr, pantilt_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    return "";
}

json ptz_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

//{"action":3,"arg":0}  这里只是左灯
json lamp_mqtt::set_light(const json& args) {
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    println("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    return "";
};

json lamp_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}
}; // namespace robot_device
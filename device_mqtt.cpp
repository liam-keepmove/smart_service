#include "device_mqtt.hpp"
#include "misc.hpp"
#include <atomic>
#include <spdlog/spdlog.h>
#include <thread>

static const char* robot_ctrl_move_topic = "/Robot/CtrlMove/500d28757093ca040eb09711";
static const char* robot_ctrl_other_topic = "/Robot/CtrlOther/500d28757093ca040eb09711";
static const char* pad_ctrl_topic = "/Pad/Ctrl/500d28757093ca040eb09711";
static const char* pantilt_ctrl_topic = "/PanTilt/Ctrl/500d28757093ca040eb09711";
static const char* robot_position_topic = "/Robot/Position/500d28757093ca040eb09711";
static const char* robot_heart_topic = "/Robot/Heart/500d28757093ca040eb09711";
static const char* robot_battery_topic = "/Robot/Battery/500d28757093ca040eb09711";

extern const char* broker_ip;
extern const int broker_port;
extern const char* broker_username; // mosquitto推荐空账号密码设置为nullptr
extern const char* broker_password; // mosquitto推荐空账号密码设置为nullptr
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
    auto heart_handler = [this]() {
        while (!heart_thread_exit) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            if (!send_heart) {
                continue;
            }
            std::string payload = fmt::format("{{\"ts\": {} }}", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_heart_topic, payload);
            int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
            if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
                spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
        }
    };
    std::thread(heart_handler).detach();
}

action_body_mqtt::~action_body_mqtt() {
    heart_thread_exit = true;
}

//{"action":0,"arg":null}
json action_body_mqtt::stop_move(const json& args) {
    json command = args;
    command["action"] = 0;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = false; // 停止心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":1,"arg":0.5}
json action_body_mqtt::speed_front_move(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = true; // 开始心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":2,"arg":0.5}
json action_body_mqtt::speed_back_move(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = true; // 开始心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

// 位置模式,到达位置后才返回,结果包含到达的位置 {"action":3,"arg":{"speed":0.5,"position":1500}}
json action_body_mqtt::location_speed_move(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    int current_location = 0;
    const int target_location = command["arg"]["position"].template get<int>();
    auto subscribe_callback = [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
        int& current_location = *(int*)obj;
        json result = json::parse((char*)message->payload);
        result.at("position").get_to(current_location);
        spdlog::info("current_location:{}", current_location);
        return 0;
    };
    json result;
    while (true) {
        if (REQ_STOP == request) {
            status = STOPPED;
            result = json::parse(fmt::format("{{\"position\": {},\"emergency_stop\": {}}}", current_location, "yes"));
        }
        if (status == RUNING) {
            if (std::abs(current_location - target_location) > 20) {
                // 距离误差仍在20mm外,继续等待
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                mosquitto_subscribe_callback(subscribe_callback, &current_location, robot_position_topic, 0, broker_ip, broker_port, "get_position", 10, true, broker_username, broker_password, nullptr, nullptr);
            } else {
                // 在20mm误差之内,意味着到达位置了,正常返回.
                result = json::parse(fmt::format("{{\"position\": {}}}", current_location));
                break;
            }
        } else if (status == STOPPED) {
            break;
        }
    }
    status = STOPPED;
    return result;
}

//{"action":4,"arg":null}
json action_body_mqtt::to_charge(const json& args) {
    status = RUNING;
    json command = args;
    command["action"] = 4;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = false; // 停止心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    std::atomic_bool is_battery_full = false;
    auto subscribe_callback = [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
        auto& is_battery_full = *(std::atomic_bool*)obj;
        int all_battery_full = 1;
        json result = json::parse((char*)message->payload);
        json battery_info_array = result.at("BatteryPck");
        for (auto& battery_info : battery_info_array) {
            if (battery_info.at("NomCap").template get<double>() == battery_info.at("RemCap").template get<double>()) {
                all_battery_full &= 1;
            } else {
                all_battery_full &= 0;
            }
        }
        if (all_battery_full == 1) {
            is_battery_full = true;
        } else {
            is_battery_full = false;
        }
        return 0;
    };
    json result;
    while (true) {
        if (REQ_STOP == request) {
            status = STOPPED;
            result = json::parse(fmt::format("{{\"emergency_stop\": {}}}", "yes"));
        }
        if (status == RUNING) {
            if (is_battery_full) {
                result = "";
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                mosquitto_subscribe_callback(subscribe_callback, &is_battery_full, robot_battery_topic, 0, broker_ip, broker_port, "get_battery", 10, true, broker_username, broker_password, nullptr, nullptr);
            }
        } else if (status == STOPPED) {
            break;
        }
    }
    status = STOPPED;
    return result;
}

//{"action":5,"arg":null}
json action_body_mqtt::motor_reset(const json& args) {
    json command = args;
    command["action"] = 5;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = false; // 停止心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":6,"arg":null}
json action_body_mqtt::restart(const json& args) {
    json command = args;
    command["action"] = 6;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = false; // 停止心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":7,"arg":1500}
json action_body_mqtt::set_current_location(const json& args) {
    json command = args;
    command["action"] = 7;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":8,"arg":0}
json action_body_mqtt::set_ultrasonic_switch(const json& args) {
    json command = args;
    command["action"] = 8;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":9,"arg":null}
json action_body_mqtt::poweroff(const json& args) {
    json command = args;
    command["action"] = 9;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_move_topic, payload);
    send_heart = false; // 停止心跳包
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_move_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":1,"arg":50}
json action_body_mqtt::set_front_light(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
}

//{"action":2,"arg":50}
json action_body_mqtt::set_back_light(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    return "";
}

//{"action":3,"arg":10}
json action_body_mqtt::set_volume(const json& args) {
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, robot_ctrl_other_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, robot_ctrl_other_topic, payload.size(), payload.c_str(), 2, false);
    return "";
}

//{"action":1,"arg":45}
json action_body_mqtt::set_left_servo(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    return "";
}

//{"action":2,"arg":45}
json action_body_mqtt::set_right_servo(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    return "";
}

void action_body_mqtt::stop() {
    if (status != STOPPED) {
        stop_move(json::parse("{\"arg\": null}"));
        request = REQ_STOP;
        while (status != STOPPED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            request = REQ_STOP;
        }
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
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, pantilt_ctrl_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, pantilt_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
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
    spdlog::info("{} send to {},payload:{}", __PRETTY_FUNCTION__, pad_ctrl_topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, pad_ctrl_topic, payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
    return "";
};

json lamp_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}
}; // namespace robot_device
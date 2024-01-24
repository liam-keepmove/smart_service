#include "device_mqtt.hpp"
#include "ThreadSafeQueue.hpp"
#include "config.hpp"
#include "misc.hpp"
#include <atomic>
#include <chrono>
#include <spdlog/spdlog.h>
#include <thread>
using namespace std::literals;
using namespace std::chrono_literals;

extern config_item global_config;
extern mosquitto* mosq;

struct mos_obj {
    ThreadSafeQueue<mosquitto_message*> msg_queue;
    std::chrono::time_point<std::chrono::steady_clock> last_recv_time; // 上次采样时间
    std::atomic_bool done = false;
};

static void mqtt_pub(const std::string& topic, const std::string& payload) {
    spdlog::info("send to {},payload:{}", topic, payload);
    int rc = mosquitto_publish(mosq, nullptr, topic.c_str(), payload.size(), payload.c_str(), 2, false);
    if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
        spdlog::info("Failed to publish message:{} ", mosquitto_strerror(rc));
        spdlog::info("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
    }
}

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

action_body_mqtt::action_body_mqtt(const std::string& id) {
    auto heart_handler = [this]() {
        while (!heart_thread_exit) {
            std::this_thread::sleep_for(2s);
            if (!send_heart) {
                continue;
            }
            std::string payload = fmt::format("{{\"ts\": {} }}", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            mqtt_pub(robot_heart_topic, payload);
        }
    };
    robot_id = id;
    robot_heart_topic += robot_id;
    robot_position_topic += robot_id;
    robot_battery_topic += robot_id;
    robot_ctrl_move_topic += robot_id;
    robot_ctrl_other_topic += robot_id;
    robot_motor_topic += robot_id;
    robot_version_topic += robot_id;
    robot_sensors_topic += robot_id;
    robot_led_topic += robot_id;
    robot_warning_topic += robot_id;

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
    send_heart = false; // 停止心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":1,"arg":0.5}
json action_body_mqtt::speed_front_move(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    send_heart = true; // 开始心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":2,"arg":0.5}
json action_body_mqtt::speed_back_move(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    send_heart = true; // 开始心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

// 位置模式,到达位置后才返回,结果包含到达的位置 {"action":3,"arg":{"speed":0.5,"position":1500}}
json action_body_mqtt::location_speed_move(const json& args) {
    status = RUNNING;
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_move_topic, payload);

    int current_location = 0;
    const int target_location = command.at("arg").at("position").template get<int>();
    mos_obj* object = new mos_obj();
    std::thread(
        &mosquitto_subscribe_callback,
        [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
            auto object = (mos_obj*)obj;
            auto elapsed = (std::chrono::duration_cast<std::chrono::seconds>)(std::chrono::steady_clock::now() - object->last_recv_time);
            spdlog::debug("elapsed is {} second.", elapsed.count());
            if (elapsed > 3s) {
                spdlog::debug("elapsed > 3s");
                auto msg = (mosquitto_message*)malloc(sizeof(mosquitto_message));
                mosquitto_message_copy(msg, message);
                object->msg_queue.push(msg);
                object->last_recv_time = std::chrono::steady_clock::now();
            } else {
                spdlog::debug("elapsed <= 3s.");
            }
            return object->done ? delete object, 1 : 0;
        },
        object, robot_position_topic.c_str(), 0, global_config.broker_ip.c_str(), global_config.broker_port, "SSGetPosition", 10, true, global_config.broker_username.c_str(), global_config.broker_password.c_str(), nullptr, nullptr)
        .detach();

    json result;
    mosquitto_message* msg;
    while (true) {
        if (REQ_STOP == request) {
            status = STOPPED;
            result = json::parse(fmt::format("{{\"position\": {},\"emergency_stop\": true}}", current_location));
            break;
        }

        if (status == RUNNING) {
            if (object->msg_queue.popTimeout(msg, 1s)) { // 之所以设置超时时间,是为了及时的响应stop请求,不能单纯的消息驱动(太快没效率,太慢不能及时响应stop)
                json msg_json = json::parse((char*)msg->payload);
                mosquitto_message_free(&msg);

                current_location = msg_json.at("position").get<int>();
                spdlog::info("current_location:{},target_location:{}", current_location, target_location);
                // 在40mm误差之内,意味着到达位置了,否则就是未到达位置.
                if (std::abs(current_location - target_location) <= 40) {
                    result = json::parse(fmt::format("{{\"position\": {}}}", current_location));
                    break;
                }
            } else {
                spdlog::debug("timeout and not recv position msg.");
            }
        }
    }
    object->done = true;
    status = STOPPED;
    return result;
}

//{"action":4,"arg":null}
json action_body_mqtt::to_charge(const json& args) {
    status = RUNNING;
    json command = args;
    command["action"] = 4;
    std::string payload = command.dump();
    send_heart = false; // 停止心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);

    mos_obj* object = new mos_obj();
    std::thread(
        &mosquitto_subscribe_callback,
        [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
            auto object = (mos_obj*)obj;
            auto elapsed = (std::chrono::duration_cast<std::chrono::seconds>)(std::chrono::steady_clock::now() - object->last_recv_time);
            if (elapsed > 10s) {
                auto msg = (mosquitto_message*)malloc(sizeof(mosquitto_message));
                mosquitto_message_copy(msg, message);
                object->msg_queue.push(msg);
                object->last_recv_time = std::chrono::steady_clock::now();
            }
            return object->done ? delete object, 1 : 0;
        },
        object, robot_battery_topic.c_str(), 0, global_config.broker_ip.c_str(), global_config.broker_port, "SSGetBattery", 10, true, global_config.broker_username.c_str(), global_config.broker_password.c_str(), nullptr, nullptr)
        .detach();

    json result;
    mosquitto_message* msg;
    while (true) {
        if (REQ_STOP == request) {
            status = STOPPED;
            result = json::parse(fmt::format("{{\"emergency_stop\": true}}"));
            break;
        }

        if (status == RUNNING) {
            if (object->msg_queue.popTimeout(msg, 1s)) {
                json msg_json = json::parse((char*)msg->payload);
                mosquitto_message_free(&msg);

                int all_battery_full = 1;
                json battery_info_array = msg_json.at("BatteryPack");
                spdlog::info("battery_info_array:");
                for (const auto& battery_info : battery_info_array) {
                    double nom_cap = battery_info.at("NomCap").template get<double>(); // 标定电池容量
                    double rem_cap = battery_info.at("RemCap").template get<double>(); // 剩余电池容量
                    spdlog::info("NomCap: {}, RemCap: {}", nom_cap, rem_cap);
                    if (std::abs(nom_cap - rem_cap) <= 0.001) {
                        all_battery_full &= 1;
                    } else {
                        all_battery_full &= 0;
                    }
                }
                if (all_battery_full == 1) {
                    result = json::parse(fmt::format("{{\"battery_full\": true}}"));
                    break;
                }
            }
        }
    }
    object->done = true;
    status = STOPPED;
    return result;
}

//{"action":5,"arg":null}
json action_body_mqtt::motor_reset(const json& args) {
    json command = args;
    command["action"] = 5;
    std::string payload = command.dump();
    send_heart = false; // 停止心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":6,"arg":null}
json action_body_mqtt::restart(const json& args) {
    json command = args;
    command["action"] = 6;
    std::string payload = command.dump();
    send_heart = false; // 停止心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    std::this_thread::sleep_for(10s); // 机器人重启,等待10s
    return json();
}

//{"action":7,"arg":1500}
json action_body_mqtt::set_current_location(const json& args) {
    json command = args;
    command["action"] = 7;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":8,"arg":0}
json action_body_mqtt::set_ultrasonic_switch(const json& args) {
    json command = args;
    command["action"] = 8;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":9,"arg":null}
json action_body_mqtt::poweroff(const json& args) {
    json command = args;
    command["action"] = 9;
    std::string payload = command.dump();
    send_heart = false; // 停止心跳包
    mqtt_pub(robot_ctrl_move_topic, payload);
    return json();
}

//{"action":1,"arg":50}
json action_body_mqtt::set_front_light(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_other_topic, payload);
    return json();
}

//{"action":2,"arg":50}
json action_body_mqtt::set_back_light(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_other_topic, payload);
    return json();
}

//{"action":3,"arg":10}
json action_body_mqtt::set_volume(const json& args) {
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    mqtt_pub(robot_ctrl_other_topic, payload);
    return json();
}

json action_body_mqtt::wait(const json& args) {
    std::this_thread::sleep_for(std::chrono::milliseconds(args.at("arg").template get<int>()));
    return json();
}

void action_body_mqtt::stop() {
    if (status == RUNNING) {
        stop_move(json::parse("{\"arg\": null}"));
        request = REQ_STOP;
        while (status == RUNNING) {
            std::this_thread::sleep_for(1s);
            request = REQ_STOP;
        }
    }
    request = INIT;
    return;
}

json action_body_mqtt::get_status() {
    json result;
    return result;
}

ptz_mqtt::ptz_mqtt(const std::string& id) {
    ptz_id = id;
    pantilt_ctrl_topic += ptz_id;
    pantilt_status_topic += ptz_id;
    pantilt_version_topic += ptz_id;
}

//{"action":1,"arg":{"yaw": 180,"pitch": 180,"roll": 180}}
json ptz_mqtt::set_xyz(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    mqtt_pub(pantilt_ctrl_topic, payload);
    std::this_thread::sleep_for(4s); // 等待云台转动到位
    return json();
}

json ptz_mqtt::set_lamp(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    mqtt_pub(pantilt_ctrl_topic, payload);
    return json();
}

json ptz_mqtt::restart(const json& args) {
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    mqtt_pub(pantilt_ctrl_topic, payload);
    std::this_thread::sleep_for(5s);
    return json();
}

json ptz_mqtt::adjust(const json& args) {
    json command = args;
    command["action"] = 4;
    std::string payload = command.dump();
    mqtt_pub(pantilt_ctrl_topic, payload);
    std::this_thread::sleep_for(5s);
    return json();
}

json ptz_mqtt::get_status() {
    json result;
    result["__PRETTY_FUNCTION__"] = __PRETTY_FUNCTION__;
    return result;
}

pad_mqtt::pad_mqtt(const std::string& id) {
    track_robot = std::make_unique<robot_device::action_body_mqtt>(id);
    pad_id = id;
    pad_ctrl_topic += pad_id;
    pad_status_topic += pad_id;
}

//{"action":1,"arg":45}
json pad_mqtt::set_left_servo(const json& args) {
    json command = args;
    command["action"] = 1;
    std::string payload = command.dump();
    mqtt_pub(pad_ctrl_topic, payload);
    std::this_thread::sleep_for(1s);
    return json();
}

//{"action":2,"arg":45}
json pad_mqtt::set_right_servo(const json& args) {
    json command = args;
    command["action"] = 2;
    std::string payload = command.dump();
    mqtt_pub(pad_ctrl_topic, payload);
    std::this_thread::sleep_for(1s);
    return json();
}

//{"action":3,"arg":45}
json pad_mqtt::set_left_lamp(const json& args) {
    json command = args;
    command["action"] = 3;
    std::string payload = command.dump();
    mqtt_pub(pad_ctrl_topic, payload);
    return json();
}

//{"action":4,"arg":45}
json pad_mqtt::set_right_lamp(const json& args) {
    json command = args;
    command["action"] = 4;
    std::string payload = command.dump();
    mqtt_pub(pad_ctrl_topic, payload);
    return json();
}
}; // namespace robot_device

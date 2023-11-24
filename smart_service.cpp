#include "device_mqtt.hpp"
#include "json.hpp"
#include "misc.hpp"
#include "mosquitto.h"
#include "robot.hpp"
#include "task.hpp"
#include <array>
#include <fmt/core.h>
using fmt::print;
using fmt::println;

json active1 = json::parse(R"(
    {
        "no": 1,
        "device_code": 1,
        "active_code": 1,
        "active_args": "{\"location\": 50, \"speed\": 50}"
    }
)");

json active2 = json::parse(R"(
    {
        "no": 2,
        "device_code": 3,
        "active_code": 1,
        "active_args": "{\"rect\": [132,123,31,11], \"name\": \"指针仪表\"}"
    }
)");

json active3 = json::parse(R"(
    {
        "no": 3,
        "device_code": 1,
        "active_code": 2,
        "active_args": "{\"direction\": \"back\", \"speed\": 50}"
    }
)");

json task_json = json::parse(R"(
{
    "id": "999666",
    "type": 3,
    "priority": 1,
    "remark": "快速巡检",
    "tag": "",
    "action_list": [
        {
            "no": 2,
            "device_code": 1,
            "active_code": 2, 
            "active_args": "{\"direction\": \"back\", \"speed\": 50}",
            "remark": "再以方向模式回来,限位器会让其自行停止",
            "tag": ""
        },
        {
            "no": 1,
            "device_code": 1,
            "active_code": 1,
            "active_args": "{\"location\": 200, \"speed\": 50}",
            "remark": "直接以位置模式去终点",
            "tag": ""
        }
    ]
}
)");

const char* broker_ip = "127.0.0.1";
const int broker_port = 1883;
const char* broker_username = "admin";  // mosquitto推荐空账号密码设置为nullptr
const char* broker_password = "public"; // mosquitto推荐空账号密码设置为nullptr
const char* mqtt_client_id = nullptr;   // mqtt session id,决定是否保留消息会话
const int broker_keep_alive = 60;       // mqtt保持连接的时间,单位秒
const std::vector<std::string> ctrl_topic_list{"/SmartSer/Pad/Ctrl", "/SmartSer/PanTilt/Ctrl"};
std::array<int, 2> sub_id = {0, 1};

void mqtt_worker() {
    int rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("Failed to init-mosquitto.") + mosquitto_strerror(rc));
    mosquitto* mosq = mosquitto_new(mqtt_client_id, true, nullptr);
    if (mosq == nullptr)
        THROW_RUNTIME_ERROR(std::string("Failed to create mosquitto object."));

    // Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
    mosquitto_connect_callback_set(mosq, [](mosquitto* mosq, void*, int reasonCode) {
        print("mqtt connack string: {}\n", mosquitto_connack_string(reasonCode));
        if (reasonCode != 0) {
            THROW_RUNTIME_ERROR(std::string("Failed to connect mosquitto because the reason code is not 0."));
        }
        print("mqtt connection is successful\n");

        for (int i = 0; i < ctrl_topic_list.size(); ++i) {
            const auto& topic = ctrl_topic_list[i];
            // Subscribe topic and add the subscribed message to the receive queue
            print("Prepare to subscribe {} topics.\n", topic);
            int rc = mosquitto_subscribe(mosq, new int(100 + i), topic.c_str(), 2);
            if (rc != MOSQ_ERR_SUCCESS) {
                THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
            }
        }
    });

    // Set the subscribe callback.  This is called when the broker responds to a subscription request.
    mosquitto_subscribe_callback_set(mosq, [](mosquitto*, void*, int mid, int qosCount, const int* grantedQos) {
        for (int i = 0; i < qosCount; ++i) {
            if (grantedQos[i] >= 0 && grantedQos[i] <= 2) {
                print("The subscription name of {} is successful, and the qos assigned by the server is {}\n", ctrl_topic_list[i], grantedQos[i]);
                println("mid is {}", mid);
            } else if (grantedQos[i] == 0x80) {
                THROW_RUNTIME_ERROR("No." + std::to_string(i) + " Subscribe failed.");
            } else {
                THROW_RUNTIME_ERROR("Error: Subscription return code exception");
            }
        }
    });

    mosquitto_message_callback_set(mosq, [](mosquitto* mosq, void*, const mosquitto_message* msg) {
        if (msg == nullptr || msg->payload == nullptr) {
            THROW_RUNTIME_ERROR(std::string("There is a problem with the mosquitto message."));
        }
        // 注意! 此回调结束后msg被清理,但由于这里是同步发送,所以不需要拷贝msg
        int rc = mosquitto_publish(mosq, nullptr, strchr(msg->topic + 1, '/'), msg->payloadlen, msg->payload, 2, false);
        if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
            println("Failed to publish message:{} ", mosquitto_strerror(rc));
            println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
        } else {
            println("转发成功");
        }
    });

    mosquitto_unsubscribe_callback_set(mosq, [](mosquitto*, void*, int) {
        THROW_RUNTIME_ERROR("Successfully unsubscribe,This should not have happened.");
    });

    // Set the disconnect callback.  This is called when the broker has received the DISCONNECT command and has disconnected the client.
    mosquitto_disconnect_callback_set(mosq, [](mosquitto*, void*, int) {
        print("The connection break callback is fired. If there is no active disconnection, it is network fluctuation!\n");
    });

#ifdef ENABLE_MOSQ_LOGGING
    mosquitto_log_callback_set(mosq, [](mosquitto*, void*, int logLevel, const char* logStr) {
        if (logLevel == MOSQ_LOG_INFO)
            fprintf(stderr, "MOSQ_LOG_INFO:%s\n", logStr);
        else if (logLevel == MOSQ_LOG_NOTICE)
            fprintf(stderr, "MOSQ_LOG_NOTICE:%s\n", logStr);
        else if (logLevel == MOSQ_LOG_WARNING)
            fprintf(stderr, "MOSQ_LOG_WARNING:%s\n", logStr);
        else if (logLevel == MOSQ_LOG_ERR)
            fprintf(stderr, "MOSQ_LOG_ERR:%s\n", logStr);
        else if (logLevel == MOSQ_LOG_DEBUG)
            fprintf(stderr, "MOSQ_LOG_DEBUG:%s\n", logStr);
        else
            fprintf(stderr, "MOSQ_LOG_UNKNOW:%s\n", logStr);
    });
#endif

    rc = mosquitto_username_pw_set(mosq, broker_username, broker_password);
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("Failed to set username and password.") + mosquitto_strerror(rc));

    println("connecting...");
    rc = mosquitto_connect(mosq, broker_ip, broker_port, broker_keep_alive);
    while (rc != MOSQ_ERR_SUCCESS) {
        println("Failed to connect mosquitto broker: {}", mosquitto_strerror(rc));
        println("reconnect after 3 seconds");
        sleep(3);
        rc = mosquitto_reconnect(mosq);
    }

    // Important! Be sure to enable the processing loop for the message network stream, which will automatically reconnect, so you must subscribe to the topic in the callback
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("mosquitto_loop_start Error: ") + mosquitto_strerror(rc));
}

int main() {
    robot mapad;
    mapad.device[1] = new robot_device::action_body_mqtt();
    mapad.device[2] = new robot_device::ptz_mqtt();
    mapad.device[3] = new robot_device::camera_mqtt();
    try {
        task check(task_json);
        check.assign_to(&mapad);
        check.run();
    } catch (json::parse_error& ex) {
        println("parse error:{}", ex.what());
    }
    for (auto& [number, device] : mapad.device) {
        delete device;
    }
    mqtt_worker();
    getchar();
}
#include "ThreadSafeQueue.hpp"
#include "device_mqtt.hpp"
#include "json.hpp"
#include "misc.hpp"
#include "mosquitto.h"
#include "robot.hpp"
#include "task.hpp"
#include <array>
#include <cstdlib>
#include <fmt/core.h>
#include <signal.h>
#include <thread>

using fmt::print;
using fmt::println;
using namespace std::chrono_literals;

mosquitto* mosq = nullptr; // 全局变量实现单例模式

class smart_service {
public:
    task current_task;
    robot bot;
    const char* broker_ip = "127.0.0.1";
    const int broker_port = 1883;
    const char* broker_username = "admin";                                                                                                                                                                         // mosquitto推荐空账号密码设置为nullptr
    const char* broker_password = "abcd1234";                                                                                                                                                                      // mosquitto推荐空账号密码设置为nullptr
    const char* mqtt_client_id = "mosquitto_hj";                                                                                                                                                                   // mqtt session id,决定是否保留消息会话
    const int broker_keep_alive = 10;                                                                                                                                                                              // mqtt心跳时间,单位秒,注意,mqtt的服务端主动断开客户端的时间是1.5倍的心跳时间
    std::array<const char*, 3> smart_ctrl_topic_list = {"/SmartSer/Pad/Ctrl/500d28757093ca040eb09711", "/SmartSer/Robot/CtrlMove/500d28757093ca040eb09711", "/SmartSer/Robot/CtrlOther/500d28757093ca040eb09711"}; // 机器人实时控制 订阅
    const char* task_recv_topic = "/SmartSer/Robot/Task/500d28757093ca040eb09711";                                                                                                                                 // 任务控制 订阅
    const char* task_feedback_topic = "/SmartSer/Robot/TaskStatus/500d28757093ca040eb09711";                                                                                                                       // 任务状态 发布
    ThreadSafeQueue<mosquitto_message*> mqtt_msg_queue;                                                                                                                                                            // mqtt消息队列

    smart_service(robot&& b)
        : bot(std::move(b)) {
    }

    ~smart_service() {
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    }

    void start_mqtt() {
        int rc = mosquitto_lib_init();
        if (rc != MOSQ_ERR_SUCCESS)
            THROW_RUNTIME_ERROR(std::string("Failed to init-mosquitto.") + mosquitto_strerror(rc));
        mosq = mosquitto_new(mqtt_client_id, true, this);
        if (mosq == nullptr)
            THROW_RUNTIME_ERROR(std::string("Failed to create mosquitto object."));

        // Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
        mosquitto_connect_callback_set(mosq, [](mosquitto* mosq, void* obj, int reasonCode) {
            smart_service* object = (smart_service*)obj;
            println("mqtt connack string: {}", mosquitto_connack_string(reasonCode));
            if (reasonCode != 0) {
                THROW_RUNTIME_ERROR(std::string("Failed to connect mosquitto because the reason code is not 0."));
            }
            println("mqtt connection is successful");

            for (const auto& topic : object->smart_ctrl_topic_list) {
                println("Prepare to subscribe {} topics.", topic);
                int rc = mosquitto_subscribe(mosq, nullptr, topic, 2);
                if (rc != MOSQ_ERR_SUCCESS) {
                    THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
                }
            }
            println("Prepare to subscribe {} topics.", object->task_recv_topic);
            int rc = mosquitto_subscribe(mosq, nullptr, object->task_recv_topic, 2);
            if (rc != MOSQ_ERR_SUCCESS) {
                THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
            }
        });

        // Set the subscribe callback.  This is called when the broker responds to a subscription request.
        mosquitto_subscribe_callback_set(mosq, [](mosquitto*, void*, int mid, int qosCount, const int* grantedQos) {
            for (int i = 0; i < qosCount; ++i) {
                if (grantedQos[i] >= 0 && grantedQos[i] <= 2) {
                    println("subscribe successful, and the qos assigned by the server is {}", grantedQos[i]);
                } else if (grantedQos[i] == 0x80) {
                    THROW_RUNTIME_ERROR("No." + std::to_string(i) + " Subscribe failed.");
                } else {
                    THROW_RUNTIME_ERROR("Error: Subscription return code exception");
                }
            }
        });

        mosquitto_message_callback_set(mosq, [](mosquitto* mosq, void* obj, const mosquitto_message* msg) {
            if (msg == nullptr || msg->payload == nullptr) {
                THROW_RUNTIME_ERROR(std::string("There is a problem with the mosquitto message."));
            }
            auto object = (smart_service*)obj;
            auto temp = (mosquitto_message*)malloc(sizeof(mosquitto_message));
            mosquitto_message_copy(temp, msg);
            object->mqtt_msg_queue.push(temp);
        });

        mosquitto_unsubscribe_callback_set(mosq, [](mosquitto*, void*, int) {
            println("Successfully unsubscribe,This should not have happened.");
        });

        // Set the disconnect callback.  This is called when the broker has received the DISCONNECT command and has disconnected the client.
        mosquitto_disconnect_callback_set(mosq, [](mosquitto*, void*, int) {
            println("The connection break callback is fired. If there is no active disconnection, it is network fluctuation!");
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
            println("reconnect after 5 seconds");
            std::this_thread::sleep_for(5s);
            rc = mosquitto_reconnect(mosq);
        }

        // Important! Be sure to enable the processing loop for the message network stream, which will automatically reconnect, so you must subscribe to the topic in the callback
        rc = mosquitto_loop_start(mosq);
        if (rc != MOSQ_ERR_SUCCESS)
            THROW_RUNTIME_ERROR(std::string("mosquitto_loop_start Error: ") + mosquitto_strerror(rc));
    }

    // 设置当前任务,设置成功返回true,失败返回false
    bool set_current_task(const task& new_task) {
        if (!current_task.is_over() && new_task.priority < current_task.priority) {
            println("fail of set current task, because the priority of new task is lower than current task");
            return false;
        };
        if (!current_task.is_over()) {
            current_task.cancel();
            println("wait for current task over...");
            while (!current_task.is_over()) {
                std::this_thread::sleep_for(200ms);
            }
        }
        current_task = new_task;
        static auto callback = [this](json task_feedback) {
            task_feedback["type"] = 8;
            std::string feedback_str = task_feedback.dump();
            int rc = mosquitto_publish(mosq, nullptr, task_feedback_topic, feedback_str.size(), feedback_str.c_str(), 2, false);
            println("send to topic \"{}\":{}", task_feedback_topic, feedback_str);
            if (rc != MOSQ_ERR_SUCCESS) {
                println("Failed to publish message:{} ", mosquitto_strerror(rc));
                println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
        };
        current_task.task_will_start_callback = callback;
        current_task.action_start_callback = callback;
        current_task.action_result_callback = callback;
        current_task.pause_callback = callback;
        current_task.cancel_callback = callback;
        current_task.over_callback = callback;
        return true;
    }

    // 后端发来的实时控制请求,取消任务后,直接转发
    void real_time_ctrl_handler(mosquitto_message* msg) {
        if (!current_task.is_over()) {
            current_task.cancel();
            while (!current_task.is_over()) {
                std::this_thread::sleep_for(200ms);
                println("wait for current task over for real time ctrl");
            }
        }
        int rc = mosquitto_publish(mosq, nullptr, strchr(msg->topic + 1, '/'), msg->payloadlen, msg->payload, 2, false);
        if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
            println("Failed to publish message:{} ", mosquitto_strerror(rc));
            println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            println("forward fail!:{}->{}", msg->topic, strchr(msg->topic + 1, '/'));
        } else {
            println("forward success:{}->{}", msg->topic, strchr(msg->topic + 1, '/'));
        }
    }

    // 后端发来的任务相关请求
    void task_handler(const std::string& task_str) {
        json task_json = json::parse(task_str);
        println("recv:\n{}", task_json.dump(4));
        json result;
        std::string result_str;
        int rc = 0;
        switch (task_json.at("type").template get<int>()) {
        case 1: // 创建一个定时任务
            // TODO: 创建定时任务
            break;
        case 2: // 删除一个定时任务
                // TODO: 删除定时任务
            break;
        case 3: // 开始即时任务
            if (set_current_task(task(task_json))) {
                std::thread(&task::run, &current_task, std::ref(bot)).detach(); // 当前任务设置成功后,立即开一个线程执行此任务
            } else {
                // 开始即时任务失败
                result["task_id"] = task_json.at("task_id");
                result["type"] = 9;
                result["status"] = 2;
                result["remark"] = "A task is currently executing, but the new task has a lower priority than the current task and cannot be set";
                result["tag"] = task_json.at("tag");
                result_str = result.dump(4);
                int rc = mosquitto_publish(mosq, nullptr, task_feedback_topic, result_str.size(), result_str.c_str(), 2, false);
                println("send to topic \"{}\":{}", task_feedback_topic, result_str);
                if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                    println("Failed to publish message:{} ", mosquitto_strerror(rc));
                    println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
                }
            }
            break;
        case 4: // 暂停当前任务
            current_task.pause();
            break;
        case 5: // 恢复当前任务
            current_task.resume();
            break;
        case 6: // 取消当前任务
            current_task.cancel();
            break;
        default: // 错误的消息类型
            result["type"] = 10;
            result["remark"] = "error message type";
            result_str = result.dump(4);
            rc = mosquitto_publish(mosq, nullptr, task_feedback_topic, result_str.size(), result_str.c_str(), 2, false);
            println("send to topic \"{}\":{}", task_feedback_topic, result_str);
            if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                println("Failed to publish message:{} ", mosquitto_strerror(rc));
                println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
            break;
        }
    }

    void mqtt_msg_handler() {
        // 串行路由各种后端来的消息
        while (true) {
            mosquitto_message* msg = nullptr;
            mqtt_msg_queue.pop(msg);
            if (std::find_if(smart_ctrl_topic_list.begin(), smart_ctrl_topic_list.end(), [msg](const char* topic) { return std::strcmp(msg->topic, topic) == 0; }) != smart_ctrl_topic_list.end()) {
                // 实时控制
                real_time_ctrl_handler(msg);
            } else if (strcmp(msg->topic, task_recv_topic) == 0) {
                if (std::strcmp((const char*)msg->payload, "debug_exit") == 0) { // debug退出方式
                    println("exit");
                    break;
                }
                // 任务控制
                task_handler((char*)(msg->payload));
            } else {
                println("unknown message:\ntopic:{}\ncontent:{}", msg->topic, (char*)msg->payload);
            }
            mosquitto_message_free(&msg); // 记得释放
        }
        // 程序结束前取消任务
        current_task.cancel();
    }
};

int main() {
    try {
        robot mapad;
        mapad.device["1"] = std::make_unique<robot_device::action_body_mqtt>();
        mapad.device["2"] = std::make_unique<robot_device::camera_mqtt>();
        mapad.device["3"] = std::make_unique<robot_device::lamp_mqtt>();
        smart_service smart_ser{std::move(mapad)};
        smart_ser.start_mqtt();
        smart_ser.mqtt_msg_handler();
    } catch (const json::parse_error& ex) {
        println("parse error:\n{}", ex.what());
    } catch (const std::bad_alloc& ex) {
        println("bad_alloc error:\n{}", ex.what());
    } catch (const std::runtime_error& ex) {
        println("runtime error:\n{}", ex.what());
    }
}
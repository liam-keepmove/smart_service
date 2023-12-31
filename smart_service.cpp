#include "ThreadSafeQueue.hpp"
#include "config.hpp"
#include "device_mqtt.hpp"
#include "json.hpp"
#include "misc.hpp"
#include "mosquitto.h"
#include "robot.hpp"
#include "task.hpp"
#include "timed_task.hpp"
#include <array>
#include <cstdlib>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <yaml-cpp/yaml.h>

using namespace std::chrono_literals;

config_item global_config("./config.yml");
mosquitto* mosq = nullptr; // 全局变量实现单例模式

class smart_service {
public:
    // 以下配置项由全局配置项合成
    std::string robot_id = global_config.robot_id;
    std::string task_recv_topic = "/SmartSer/Robot/Task/" + robot_id;
    std::string task_feedback_topic = "/SmartSer/Robot/TaskStatus/" + robot_id;
    std::string task_debug_topic = "/SmartSer/Debug";
    std::vector<std::string> forward_topics = {
        "/SmartSer/Robot/Heart/" + robot_id,
        "/SmartSer/Robot/CtrlMove/" + robot_id,
        "/SmartSer/Robot/CtrlOther/" + robot_id,
    };
    timed_task_set timed_set{"/etc/cron.d/timed_task_robot", task_recv_topic}; // crontab的定时任务文件
    task current_task;
    robot bot;
    ThreadSafeQueue<mosquitto_message*> mqtt_msg_queue; // mqtt消息队列

    smart_service(robot&& b)
        : bot(std::move(b)) {
        for (const auto& module : global_config.modules) {
            if (module.type == "PanTilt") {
                forward_topics.emplace_back("/SmartSer/PanTilt/Ctrl/" + module.id);
            } else if (module.type == "Pad") {
                forward_topics.emplace_back("/SmartSer/Pad/Ctrl/" + module.id);
            } else {
                spdlog::warn("unknow module info,type:{},name:{},id:{},Unable to forward this topic,ignored", module.type, module.name, module.id);
            }
        }
    }

    ~smart_service() {
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    }

    void start_mqtt() {
        int rc = mosquitto_lib_init();
        if (rc != MOSQ_ERR_SUCCESS)
            THROW_RUNTIME_ERROR(std::string("Failed to init-mosquitto.") + mosquitto_strerror(rc));
        mosq = mosquitto_new(global_config.mqtt_client_id.c_str(), true, this);
        if (mosq == nullptr)
            THROW_RUNTIME_ERROR(std::string("Failed to create mosquitto object."));

        // Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
        mosquitto_connect_callback_set(mosq, [](mosquitto* mosq, void* obj, int reasonCode) {
            spdlog::info("mqtt connack string: {}", mosquitto_connack_string(reasonCode));
            if (reasonCode != 0) {
                THROW_RUNTIME_ERROR(std::string("Failed to connect mosquitto because the reason code is not 0."));
            }
            spdlog::info("mqtt connection is successful");
            auto object = (smart_service*)obj;
            for (const auto& topic : object->forward_topics) {
                spdlog::info("Prepare to subscribe {} topics.", topic);
                int rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 2);
                if (rc != MOSQ_ERR_SUCCESS) {
                    THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
                }
            }
            spdlog::info("Prepare to subscribe {} topics.", object->task_recv_topic);
            int rc = mosquitto_subscribe(mosq, nullptr, object->task_recv_topic.c_str(), 2);
            if (rc != MOSQ_ERR_SUCCESS) {
                THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
            }
            spdlog::info("Prepare to subscribe {} topics.", object->task_debug_topic);
            rc = mosquitto_subscribe(mosq, nullptr, object->task_debug_topic.c_str(), 2);
            if (rc != MOSQ_ERR_SUCCESS) {
                THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
            }
        });

        // Set the subscribe callback.  This is called when the broker responds to a subscription request.
        mosquitto_subscribe_callback_set(mosq, [](mosquitto*, void*, int mid, int qosCount, const int* grantedQos) {
            for (int i = 0; i < qosCount; ++i) {
                if (grantedQos[i] >= 0 && grantedQos[i] <= 2) {
                    spdlog::info("subscribe successful, and the qos assigned by the server is {}", grantedQos[i]);
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
            spdlog::info("Successfully unsubscribe,This should not have happened.");
        });

        // Set the disconnect callback.  This is called when the broker has received the DISCONNECT command and has disconnected the client.
        mosquitto_disconnect_callback_set(mosq, [](mosquitto*, void*, int) {
            spdlog::info("The connection break callback is fired. If there is no active disconnection, it is network fluctuation!");
        });

#ifdef ENABLE_MOSQ_LOGGING
        mosquitto_log_callback_set(mosq, [](mosquitto*, void*, int logLevel, const char* logStr) {
            if (logLevel == MOSQ_LOG_INFO)
                spdlog::info(stderr, "MOSQ_LOG_INFO:%s\n", logStr);
            else if (logLevel == MOSQ_LOG_NOTICE)
                spdlog::info(stderr, "MOSQ_LOG_NOTICE:%s\n", logStr);
            else if (logLevel == MOSQ_LOG_WARNING)
                spdlog::info(stderr, "MOSQ_LOG_WARNING:%s\n", logStr);
            else if (logLevel == MOSQ_LOG_ERR)
                spdlog::info(stderr, "MOSQ_LOG_ERR:%s\n", logStr);
            else if (logLevel == MOSQ_LOG_DEBUG)
                spdlog::info(stderr, "MOSQ_LOG_DEBUG:%s\n", logStr);
            else
                spdlog::info(stderr, "MOSQ_LOG_UNKNOW:%s\n", logStr);
        });
#endif

        rc = mosquitto_username_pw_set(mosq, global_config.broker_username.c_str(), global_config.broker_password.c_str());
        if (rc != MOSQ_ERR_SUCCESS)
            THROW_RUNTIME_ERROR(std::string("Failed to set username and password.") + mosquitto_strerror(rc));

        spdlog::info("connecting...");
        rc = mosquitto_connect(mosq, global_config.broker_ip.c_str(), global_config.broker_port, global_config.broker_keep_alive);
        while (rc != MOSQ_ERR_SUCCESS) {
            spdlog::error("Failed to connect mosquitto broker: {}", mosquitto_strerror(rc));
            spdlog::error("reconnect after 5 seconds");
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
            spdlog::info("fail of set current task, because the priority of new task is lower than current task");
            return false;
        };
        if (!current_task.is_empty()) {
            if (!current_task.is_over()) {
                current_task.cancel();
            }
        }
        current_task = new_task;
        static auto callback = [this](json task_feedback) {
            task_feedback["type"] = 8;
            std::string feedback_str = task_feedback.dump();
            int rc = mosquitto_publish(mosq, nullptr, task_feedback_topic.c_str(), feedback_str.size(), feedback_str.c_str(), 2, false);
            spdlog::info("send to topic \"{}\":{}", task_feedback_topic, feedback_str);
            if (rc != MOSQ_ERR_SUCCESS) {
                spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
                spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
            if (current_task.is_over()) {
                // 任务结束后,定时任务转换来的即时任务如果执行次数足够了,要删除掉,执行次数不够则要更新执行次数
                for (const auto& timed_task : timed_set.get_timed_task_list()) {
                    if (timed_task.at("id") == current_task.id) { // 当前任务是一个定时任务转换而来的
                        if (current_task.executed_count >= current_task.max_count) {
                            timed_set.del_timed_task(current_task.id);
                        } else {
                            json update_task = timed_task;
                            update_task.at("executed_count") = current_task.executed_count;
                            timed_set.add_timed_task(update_task);
                        }
                        break;
                    }
                }
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
        }
        spdlog::info("forward:{}->{}", msg->topic, strchr(msg->topic + 1, '/'));
        int rc = mosquitto_publish(mosq, nullptr, strchr(msg->topic + 1, '/'), msg->payloadlen, msg->payload, 2, false);
        if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
            spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
            spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
        }
    }

    // 后端发来的任务相关请求
    void task_handler(json task_json) {
        spdlog::info("task handler recv:\n{}", task_json.dump(4));
        json result;
        std::string result_str;
        int rc = 0;
        switch (task_json.at("type").template get<int>()) {
        case 1: // 创建一个定时任务
            timed_set.add_timed_task(task_json);
            result["task_id"] = task_json.at("id");
            result["type"] = 9;
            result["status"] = 1;
            result["remark"] = "success of add timed_task";
            result["tag"] = task_json.at("tag");
            result_str = result.dump(4);
            rc = mosquitto_publish(mosq, nullptr, task_feedback_topic.c_str(), result_str.size(), result_str.c_str(), 2, false);
            spdlog::info("send to topic \"{}\":{}", task_feedback_topic, result_str);
            if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
                spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
            break;
        case 2: // 删除一个定时任务
            timed_set.del_timed_task(task_json.at("dest_task_id").template get<std::string>());
            result["task_id"] = task_json.at("dest_task_id");
            result["type"] = 9;
            result["status"] = 1;
            result["remark"] = "success of delete timed_task";
            result["tag"] = task_json.at("tag");
            result_str = result.dump(4);
            rc = mosquitto_publish(mosq, nullptr, task_feedback_topic.c_str(), result_str.size(), result_str.c_str(), 2, false);
            spdlog::info("send to topic \"{}\":{}", task_feedback_topic, result_str);
            if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
                spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
            break;
        case 3: // 开始即时任务
            if (set_current_task(task(task_json))) {
                std::thread(&task::run, &current_task, std::ref(bot)).detach(); // 当前任务设置成功后,立即开一个线程执行此任务
            } else {
                // 开始即时任务失败
                result["task_id"] = task_json.at("id");
                result["type"] = 9;
                result["status"] = 2;
                result["remark"] = "A task is currently executing, but the new task has a lower priority than the current task and cannot be set";
                result["tag"] = task_json.at("tag");
                result_str = result.dump(4);
                int rc = mosquitto_publish(mosq, nullptr, task_feedback_topic.c_str(), result_str.size(), result_str.c_str(), 2, false);
                spdlog::info("send to topic \"{}\":{}", task_feedback_topic, result_str);
                if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                    spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
                    spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
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
            rc = mosquitto_publish(mosq, nullptr, task_feedback_topic.c_str(), result_str.size(), result_str.c_str(), 2, false);
            spdlog::info("send to topic \"{}\":{}", task_feedback_topic, result_str);
            if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
                spdlog::error("Failed to publish message:{} ", mosquitto_strerror(rc));
                spdlog::error("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
            }
            break;
        }
    }

    void mqtt_msg_handler() {
        // 串行路由各种后端来的消息,必须串行,因为消息之间有上下文关联,例如先有暂停消息,才能处理恢复消息
        while (true) {
            mosquitto_message* msg = nullptr;
            mqtt_msg_queue.pop(msg);
            if (std::find_if(forward_topics.begin(), forward_topics.end(), [msg](const std::string& topic) { return std::strcmp(msg->topic, topic.c_str()) == 0; }) != forward_topics.end()) {
                // 实时控制
                real_time_ctrl_handler(msg);
            } else if (strcmp(msg->topic, task_recv_topic.c_str()) == 0) {
                // 任务控制
                try {
                    task_handler(json::parse((char*)(msg->payload)));
                } catch (const json::parse_error& ex) {
                    spdlog::info("throw line:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                } catch (const json::out_of_range& ex) {
                    spdlog::info("throw line:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                } catch (const json::type_error& ex) {
                    spdlog::info("throw line:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                }
            } else if (strcmp(msg->topic, task_debug_topic.c_str()) == 0) {
                if (std::strcmp((const char*)msg->payload, "debug_exit") == 0) { // debug退出方式
                    spdlog::info("exit");
                    break;
                }
            } else {
                spdlog::warn("unknown message:\ntopic:{}\ncontent:{}", msg->topic, (char*)msg->payload);
            }
            mosquitto_message_free(&msg); // 记得释放
        }
        // 程序结束前取消任务
        current_task.cancel();
    }
};

int main() {
    try {
        robot bot;
        bot.add_device(global_config.robot_id, std::make_unique<robot_device::action_body_mqtt>(global_config.robot_id));
        spdlog::info("add_device:action_body,name:Robot,id:{}", global_config.robot_id);
        for (const auto& module : global_config.modules) {
            if (module.type == "PanTilt") {
                bot.add_device(module.name, std::make_unique<robot_device::ptz_mqtt>(module.id));
                spdlog::info("add_device:{},name:{},id:{}", module.type, module.name, module.id);
            } else if (module.type == "Pad") {
                bot.add_device(module.name, std::make_unique<robot_device::pad_mqtt>(module.id));
                spdlog::info("add_device:{},name:{},id:{}", module.type, module.name, module.id);
            } else {
                spdlog::warn("unknow module info,type:{},name:{},id:{},Can't be combined into a robot,ignored", module.type, module.name, module.id);
            }
        }
        smart_service smart_ser{std::move(bot)};
        smart_ser.start_mqtt();
        smart_ser.mqtt_msg_handler();
    } catch (const std::bad_alloc& ex) {
        spdlog::info("bad_alloc error:\n{}", ex.what());
    } catch (const std::runtime_error& ex) {
        spdlog::info("runtime error:\n{}", ex.what());
    }
    //     std::string json_str = R"(
    //         {
    //             "Base64Img": "",
    //             "Alg": [
    //                 {
    //                     "Cls": [4, 5],
    //                     "Range": 3,
    //                     "Conf": 0.8,
    //                     "Rect": [63,165,219,339],
    //                     "Name": "指针仪表",
    //                     "DeviceCode": "123456",
    //                     "ModelCode": "KeypointMeter"
    //                 }
    //             ]
    //         }
    //     )";
    //     cv::Mat image = cv::imread("test.jpg");
    //     auto base64 = img_to_base64(image);
    //     auto json_config = nlohmann::json::parse(json_str);
    //     json_config["Base64Img"] = base64;
    //     auto result = detect_image(json_config);
    //     fmt::println("{}", result.dump(4));
    //     cv::imwrite("output.jpg", base64_to_img(result["Base64Img"]));
}

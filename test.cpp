#include "image_detect.hpp"
#include "json.hpp"
#include "misc.hpp"
#include <curl/curl.h>
#include <fmt/core.h>
#include <iostream>
#include <mosquitto.h>
#include <string_view>
using fmt::print;
using fmt::println;

const char* brokerIP = "192.168.2.167";
const int brokerPort = 1883;
const char* brokerUsername = "admin";  // mosquitto推荐空账号密码设置为nullptr
const char* brokerPassword = "public"; // mosquitto推荐空账号密码设置为nullptr
const char* mqttSessionID = nullptr;   // mqtt session id,决定是否保留消息会话
const int brokerKeepAlive = 60;        // mqtt保持连接的时间,单位秒

/*
void mqttWorker() {
    int rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("Failed to init-mosquitto.") + mosquitto_strerror(rc));
    mosquitto* mosq = mosquitto_new(mqttSessionID, true, nullptr);
    if (mosq == nullptr)
        THROW_RUNTIME_ERROR(std::string("Failed to create mosquitto object."));

    // Set the connect callback.  This is called when the broker sends a CONNACK message in response to a connection.
    mosquitto_connect_callback_set(mosq, [](mosquitto* mosq, void*, int reasonCode) {
        print("mqtt connack string: {}\n", mosquitto_connack_string(reasonCode));
        if (reasonCode != 0) {
            THROW_RUNTIME_ERROR(std::string("Failed to connect mosquitto because the reason code is not 0."));
        }
        print("mqtt connection is successful\n");

        // Subscribe topic and add the subscribed message to the receive queue
        print("Prepare to subscribe {} topics.\n", ctrlTopicName);
        int rc = mosquitto_subscribe(mosq, nullptr, ctrlTopicName, RecvQoS);
        if (rc != MOSQ_ERR_SUCCESS) {
            THROW_RUNTIME_ERROR(std::string("Subscription failure:") + mosquitto_strerror(rc));
        }
    });

    // Set the subscribe callback.  This is called when the broker responds to a subscription request.
    mosquitto_subscribe_callback_set(mosq, [](mosquitto*, void*, int, int qosCount, const int* grantedQos) {
        for (int i = 0; i < qosCount; ++i) {
            if (grantedQos[i] >= 0 && grantedQos[i] <= 2) {
                print("The subscription of No.{} is successful, and the qos assigned by the server is {}\n", i, grantedQos[i]);
            } else if (grantedQos[i] == 0x80) {
                THROW_RUNTIME_ERROR("No." + std::to_string(i) + " Subscribe failed.");
            } else {
                THROW_RUNTIME_ERROR("Error: Subscription return code exception");
            }
        }
        print("topic({}) subscription is successful\n", ctrlTopicName);
    });

    mosquitto_message_callback_set(mosq, [](mosquitto*, void*, const mosquitto_message* msg) {
        if (msg == nullptr || msg->payload == nullptr) {
            THROW_RUNTIME_ERROR(std::string("There is a problem with the mosquitto message."));
        }
        println("recv msg");
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

    rc = mosquitto_username_pw_set(mosq, brokerUsername, brokerPassword);
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("Failed to set username and password.") + mosquitto_strerror(rc));

    println("connecting...");
    rc = mosquitto_connect(mosq, brokerIP, brokerPort, brokerKeepAlive);
    while (rc != MOSQ_ERR_SUCCESS) {
        println("Failed to connect mosquitto broker: {}", mosquitto_strerror(rc)));
        println("reconnect after 3 seconds");
        sleep(3);
        rc = mosquitto_reconnect(mosq);
    }

    // Important! Be sure to enable the processing loop for the message network stream, which will automatically reconnect, so you must subscribe to the topic in the callback
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS)
        THROW_RUNTIME_ERROR(std::string("mosquitto_loop_start Error: ") + mosquitto_strerror(rc));
    std::string statusJsonStr;
    while (true) {
        rc = mosquitto_publish(mosq, nullptr, statusTopicName, statusJsonStr.size(), statusJsonStr.c_str(), SendQoS, false);
        if (rc != MOSQ_ERR_SUCCESS) { // if the resulting packet would be larger than supported by the broker.
            println("Failed to publish message:{} ", mosquitto_strerror(rc));
            println("Wait for \"mosquitto_loop_start()\" function to reconnect automatically");
        }
    }
}
*/

using json = nlohmann::ordered_json;

struct timed_task {
    std::string cron_expr;
    std::string username;
    std::string command;

    timed_task(std::string cron_expr, std::string username, std::string command)
        : cron_expr(cron_expr), username(username), command(command) {
    }

    std::string to_cron_str() {
        return fmt::format("{cron_expr} {username} {command}", fmt::arg("cron_expr", cron_expr), fmt::arg("username", username), fmt::arg("command", command));
    }

    friend void to_json(json& j, const timed_task& t) {
        j = json{{"cron_expr", t.cron_expr}, {"username", t.username}, {"command", t.command}};
    }

    friend void from_json(const json& j, timed_task& t) {
        j.at("cron_expr").get_to(t.cron_expr);
        j.at("username").get_to(t.username);
        j.at("command").get_to(t.command);
    }
};

struct timed_task_set {
public:
    std::vector<timed_task> timed_task_list;

    timed_task_set(json timed_task_json) {
    }

    friend void to_json(json& j, const timed_task_set& ts) {
        json temp;
        for (const auto& task : ts.timed_task_list) {
            temp.emplace_back(json::parse(task));
        }
        j["timed_task_list"] = temp;
    }

    friend void from_json(const json& j, timed_task_set& ts) {
        ts.timed_task_list.clear();
        for (const auto& task : j.at("timed_task_list")) {
            ts.timed_task_list.emplace_back(task);
        }
    }
};

void timed_task_create(nlohmann::json task) {
    std::string content = R"({"key": "Test"})";
    std::string cron = "* * * * *";
    std::string task_id = "3a0eaaf2-7464-30d0-16f2-81d31620ae39";
    std::string dirPath = "./timed_task";
    std::string task_file_name = task_id + ".json";
    std::string filePath = dirPath + "/" + task_file_name;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.path().filename() == task_file_name) {
            throw std::runtime_error("File already exists: " + filePath);
        }
    }
    std::ofstream task_file(filePath);
    if (!task_file.is_open()) {
        throw std::runtime_error("Unable to open file for writing: " + filePath);
    }
    task_file << content;
    task_file.close();
    std::string command = fmt::format("cat ${{TIMED_TASK_PATH}}/{task_file_name} > ${{TIMED_TASK_PATH}}/current.json && killall -s SIGUSR1 test.out", fmt::arg("task_file_name", task_file_name));
    std::string str = fmt::format("{cron_expr} {username} {command}", fmt::arg("cron_expr", "* * * * *"), fmt::arg("username", "root"), fmt::arg("command", command));
    std::string cronFilePath = "/etc/cron.d/timed_task_robot";
    std::string additionalContent = str;
    if (std::filesystem::is_regular_file(cronFilePath)) {
        try {
            std::ofstream cron_file(cronFilePath, std::ios::app);
            if (!cron_file.is_open()) {
                throw std::runtime_error("Unable to open file for writing: " + cronFilePath);
            }
            cron_file << str << std::endl;
            cron_file.close();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    } else {
        try {
            std::ofstream cron_file(cronFilePath);
            if (!cron_file.is_open()) {
                throw std::runtime_error("Unable to open file for writing: " + cronFilePath);
            }
            cron_file << "SHELL=/bin/sh" << std::endl;
            cron_file << "PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin" << std::endl;
            cron_file << "TIMED_TASK_PATH=" << std::filesystem::current_path() / "timed_task" << std::endl;
            cron_file << str << std::endl;
            cron_file.close();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

int main() {
    //    std::string json_str = R"(
    //        {
    //            "Base64Img": "",
    //            "Alg": [
    //                {
    //                    "Cls": [4, 5],
    //                    "Range": 3,
    //                    "Conf": 0.8,
    //                    "Rect": [63,165,219,339],
    //                    "Name": "指针仪表",
    //                    "DeviceCode": "123456",
    //                    "ModelCode": "KeypointMeter"
    //                }
    //            ]
    //        }
    //    )";
    //    cv::Mat image = cv::imread("test.jpg");
    //    auto base64 = img_to_base64(image);
    //    auto json_config = nlohmann::json::parse(json_str);
    //    json_config["Base64Img"] = base64;
    //    auto result = detect_image(json_config);
    //    fmt::println("{}", result.dump(4));
    //    cv::imwrite("output.jpg", base64_to_img(result["Base64Img"]));
}
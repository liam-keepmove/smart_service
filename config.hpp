#pragma once
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// 配置项结构体
struct config_item {
private:
    // 组件结构体
    struct module {
        std::string module_name;
        std::string module_id;
    };

public:
    std::string broker_ip;
    int broker_port;
    std::string broker_username;
    std::string broker_password;
    int broker_keep_alive;
    std::string mqtt_client_id;
    std::string robot_id;
    std::vector<module> modules;

    config_item(const std::string& filename) {
        try {
            YAML::Node node = YAML::LoadFile(filename);
            broker_ip = node["broker_ip"].as<std::string>();
            broker_port = node["broker_port"].as<int>();
            broker_username = node["broker_username"].as<std::string>();
            broker_password = node["broker_password"].as<std::string>();
            broker_keep_alive = node["broker_keep_alive"].as<int>();
            mqtt_client_id = node["mqtt_client_id"].as<std::string>();
            robot_id = node["robot_id"].as<std::string>();
            const YAML::Node& modules_node = node["modules"];
            for (const auto& module_node : modules_node) {
                module module;
                module.module_name = module_node["module_name"].as<std::string>();
                module.module_id = module_node["module_id"].as<std::string>();
                modules.push_back(module);
            }

        } catch (const YAML::Exception& e) {
            spdlog::error("Error reading YAML config file: {}", e.what());
        }
    }
};
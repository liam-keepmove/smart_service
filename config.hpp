#pragma once
#include <set>

// 配置项结构体
struct config_item {
private:
    // 组件结构体
    struct module {
        std::string type;
        std::string name;
        std::string id;

        bool operator<(const module& other) const;
    };

public:
    std::string broker_ip;
    int broker_port;
    std::string broker_username;
    std::string broker_password;
    int broker_keep_alive;
    std::string mqtt_client_id;
    std::string robot_type;
    std::string robot_id;
    int battery_threshold;
    std::set<module> modules;

    config_item(const std::string& filename);
};
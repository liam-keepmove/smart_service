#pragma once

#include "json.hpp"
#include <functional>
#include <map>

namespace robot_device {

class device {
public:
    virtual std::function<json(const json&)> get_action(int action_num) = 0;
};

class camera : public device {
public:
    // 检测螺栓
    virtual json detect_bolt(const json& args) = 0;

    // 检测仪表
    virtual json detect_meter(const json& args) = 0;

    // 检测拉杆
    virtual json detect_pull_rod(const json& args) = 0;

    std::function<json(const json&)> get_action(int action_num) override {
        return action_map.at(action_num);
    }

private:
    // 截图
    virtual json shot() = 0;

    // 动作表
    std::map<int, std::function<json(const json&)>> action_map{
        {1, std::bind(&camera::detect_bolt, this, std::placeholders::_1)},
        {2, std::bind(&camera::detect_meter, this, std::placeholders::_1)},
        {3, std::bind(&camera::detect_pull_rod, this, std::placeholders::_1)},
    };
};

class action_body : public device {
public:
    // 速度和方向模式
    virtual json direct_speed_move(const json& args) = 0;

    // 位置模式
    virtual json location_speed_move(const json& args) = 0;

    std::function<json(const json&)> get_action(int action_num) {
        return action_map.at(action_num);
    }

private:
    std::map<int, std::function<json(const json&)>> action_map{
        {1, std::bind(&action_body::direct_speed_move, this, std::placeholders::_1)},
        {2, std::bind(&action_body::location_speed_move, this, std::placeholders::_1)},
    };
    virtual json get_status() = 0;
};

class ptz : public device {
public:
    // 设置云台xyz
    virtual json set_xyz(const json& args) = 0;

    std::function<json(const json&)> get_action(int action_num) {
        return action_map.at(action_num);
    }

private:
    std::map<int, std::function<json(const json&)>> action_map{
        {1, std::bind(&ptz::set_xyz, this, std::placeholders::_1)},
    };

    virtual json get_status() = 0;
};

class lamp : public device {
public:
    // 设置灯亮度
    virtual json set_light(const json& args) = 0;

    std::function<json(const json&)> get_action(int action_num) {
        return action_map.at(action_num);
    }

private:
    std::map<int, std::function<json(const json&)>> action_map{
        {1, std::bind(&lamp::set_light, this, std::placeholders::_1)},
    };

    virtual json get_status() = 0;
};

} // namespace robot_device
#pragma once

#include "json.hpp"
#include <functional>
#include <map>

namespace robot_device {

class device {
public:
    // 所有设备都需要有暂停函数,用于任务暂停
    virtual void stop() {}

    // 所有设备都需要有初始化函数,用于任务开始前的初始化
    virtual void init() {}

    virtual std::function<json(const json&)> get_action(std::string action_code) = 0;
};

class camera : public device {
public:
    // 检测螺栓
    virtual json detect_bolt(const json& args) = 0;

    // 检测仪表
    virtual json detect_meter(const json& args) = 0;

    // 检测拉杆
    virtual json detect_pull_rod(const json& args) = 0;

    std::function<json(const json&)> get_action(std::string action_code) override {
        return action_map.at(action_code);
    }

private:
    // 截图
    virtual json shot() = 0;

    // 动作表
    std::map<std::string, std::function<json(const json&)>> action_map{
        {"1", std::bind(&camera::detect_bolt, this, std::placeholders::_1)},
        {"2", std::bind(&camera::detect_meter, this, std::placeholders::_1)},
        {"3", std::bind(&camera::detect_pull_rod, this, std::placeholders::_1)},
    };
};

class action_body : public device {
public:
    // 停止移动
    virtual json stop_move(const json& args) = 0;

    // 速度模式向前
    virtual json speed_front_move(const json& args) = 0;

    // 速度模式向后
    virtual json speed_back_move(const json& args) = 0;

    // 位置模式
    virtual json location_speed_move(const json& args) = 0;

    // 回程充电
    virtual json to_charge(const json& args) = 0;

    // 电机复位
    virtual json motor_reset(const json& args) = 0;

    // 机器人重启
    virtual json restart(const json& args) = 0;

    // 设置当前位置,为了校准位置
    virtual json set_current_location(const json& args) = 0;

    // 设置超声波开关
    virtual json set_ultrasonic_switch(const json& args) = 0;

    // 关机
    virtual json poweroff(const json& args) = 0;

    // 控制其他-前灯
    virtual json set_front_light(const json& args) = 0;

    // 控制其他-后灯
    virtual json set_back_light(const json& args) = 0;

    // 音量调节
    virtual json set_volume(const json& args) = 0;

    // 等待
    virtual json wait(const json& args) = 0;

    std::function<json(const json&)> get_action(std::string action_code) {
        return action_map.at(action_code);
    }

private:
    std::map<std::string, std::function<json(const json&)>> action_map{
        {"0", std::bind(&action_body::stop_move, this, std::placeholders::_1)},
        {"1", std::bind(&action_body::speed_front_move, this, std::placeholders::_1)},
        {"2", std::bind(&action_body::speed_back_move, this, std::placeholders::_1)},
        {"TrackRobotMovePosition", std::bind(&action_body::location_speed_move, this, std::placeholders::_1)},
        {"TrackRobotCharge", std::bind(&action_body::to_charge, this, std::placeholders::_1)},
        {"5", std::bind(&action_body::motor_reset, this, std::placeholders::_1)},
        {"6", std::bind(&action_body::restart, this, std::placeholders::_1)},
        {"7", std::bind(&action_body::set_current_location, this, std::placeholders::_1)},
        {"8", std::bind(&action_body::set_ultrasonic_switch, this, std::placeholders::_1)},
        {"9", std::bind(&action_body::poweroff, this, std::placeholders::_1)},
        {"TrackRobotCtrlPreLed", std::bind(&action_body::set_front_light, this, std::placeholders::_1)},
        {"TrackRobotCtrlBackLed", std::bind(&action_body::set_back_light, this, std::placeholders::_1)},
        {"12", std::bind(&action_body::set_volume, this, std::placeholders::_1)},
        {"Wait", std::bind(&action_body::wait, this, std::placeholders::_1)},
    };
    virtual json get_status() = 0;
};

class ptz : public device {
public:
    // 设置云台xyz
    virtual json set_xyz(const json& args) = 0;

    // 设置补光灯
    virtual json set_lamp(const json& args) = 0;

    // 重启
    virtual json restart(const json& args) = 0;

    // 校准
    virtual json adjust(const json& args) = 0;

    std::function<json(const json&)> get_action(std::string action_code) {
        return action_map.at(action_code);
    }

private:
    std::map<std::string, std::function<json(const json&)>> action_map{
        {"PanTiltCtrl", std::bind(&ptz::set_xyz, this, std::placeholders::_1)},
        {"PanTiltCtrlLed", std::bind(&ptz::set_lamp, this, std::placeholders::_1)},
        {"3", std::bind(&ptz::restart, this, std::placeholders::_1)},
        {"4", std::bind(&ptz::adjust, this, std::placeholders::_1)},
    };

    virtual json get_status() = 0;
};

class pad : public device {
public:
    // 左舵机控制
    virtual json set_left_servo(const json& args) = 0;

    // 右舵机控制
    virtual json set_right_servo(const json& args) = 0;

    // 设置左补光灯
    virtual json set_left_lamp(const json& args) = 0;

    // 设置右补光灯
    virtual json set_right_lamp(const json& args) = 0;

    std::function<json(const json&)> get_action(std::string action_code) {
        return action_map.at(action_code);
    }

private:
    std::map<std::string, std::function<json(const json&)>> action_map{
        {"1", std::bind(&pad::set_left_servo, this, std::placeholders::_1)},
        {"2", std::bind(&pad::set_right_servo, this, std::placeholders::_1)},
        {"3", std::bind(&pad::set_left_lamp, this, std::placeholders::_1)},
        {"4", std::bind(&pad::set_right_lamp, this, std::placeholders::_1)},
    };
};

} // namespace robot_device
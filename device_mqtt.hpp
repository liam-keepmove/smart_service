#pragma once

#include "device.hpp"
#include <atomic>
#include <mosquitto.h>

namespace robot_device {
class camera_mqtt : public camera {
public:
    json detect_bolt(const json& args) override;

    json detect_meter(const json& args) override;

    json detect_pull_rod(const json& args) override;

private:
    json shot() override;
};

class action_body_mqtt : public action_body {
public:
    enum { RUNING = 0,
           STOPPED = 1,
           REQ_STOP = 2,
    };

private:
    std::atomic_int request = STOPPED;
    std::atomic_int status = STOPPED;
    std::atomic_bool heart_thread_exit = false;
    std::atomic_bool send_heart = false; // 默认不发送心跳包
    void heart_handler();

public:
    action_body_mqtt();
    ~action_body_mqtt();

    json stop_move(const json& args) override;

    json speed_front_move(const json& args) override;

    json speed_back_move(const json& args) override;

    // 位置模式
    json location_speed_move(const json& args) override;

    // 回程充电
    json to_charge(const json& args) override;

    // 电机复位
    json motor_reset(const json& args) override;

    // 机器人重启
    json restart(const json& args) override;

    // 设置当前位置,为了校准位置
    json set_current_location(const json& args) override;

    // 设置超声波开关
    json set_ultrasonic_switch(const json& args) override;

    // 关机
    json poweroff(const json& args) override;

    // 控制其他-前灯
    json set_front_light(const json& args) override;

    // 控制其他-后灯
    json set_back_light(const json& args) override;

    // 音量调节
    json set_volume(const json& args) override;

    // 左舵机控制
    json set_left_servo(const json& args) override;

    // 右舵机控制
    json set_right_servo(const json& args) override;

    void stop() override;

private:
    json get_status() override;
};

class ptz_mqtt : public ptz {
public:
    json set_xyz(const json& args) override;

private:
    json get_status() override;
};

class lamp_mqtt : public lamp {
public:
    json set_light(const json& args) override;

private:
    json get_status() override;
};

}; // namespace robot_device
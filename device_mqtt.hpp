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
    enum { INIT = -1,
           RUNNING = 0,
           STOPPED = 1,
           REQ_STOP = 2,
    };

private:
    std::string robot_id;
    std::string robot_heart_topic = "/Robot/Heart/";
    std::string robot_motor_topic = "/Robot/Motor/";
    std::string robot_position_topic = "/Robot/Position/";
    std::string robot_version_topic = "/Robot/Version/";
    std::string robot_sensors_topic = "/Robot/Sensors/";
    std::string robot_led_topic = "/Robot/Led/";
    std::string robot_battery_topic = "/Robot/Battery/";
    std::string robot_warning_topic = "/Robot/Warning/";
    std::string robot_ctrl_move_topic = "/Robot/CtrlMove/";
    std::string robot_ctrl_other_topic = "/Robot/CtrlOther/";

    std::atomic_int request = INIT;
    std::atomic_int status = STOPPED;
    std::atomic_bool heart_thread_exit = false;
    std::atomic_bool send_heart = false; // 默认不发送心跳包

public:
    action_body_mqtt(const std::string& id);

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

    void stop() override;

private:
    json get_status() override;
};

class ptz_mqtt : public ptz {
private:
    std::string ptz_id;
    std::string pantilt_ctrl_topic = "/PanTilt/Ctrl/";
    std::string pantilt_status_topic = "/PanTilt/BasicState/";
    std::string pantilt_version_topic = "/PanTilt/Version/";

public:
    ptz_mqtt(const std::string& id);

    json set_xyz(const json& args) override;

    json set_lamp(const json& args) override;

    json restart(const json& args) override;

    json adjust(const json& args) override;

private:
    json get_status() override;
};

class pad_mqtt : public pad {
private:
    std::string pad_id;
    std::string pad_ctrl_topic = "/Pad/Ctrl/";
    std::string pad_status_topic = "/Pad/Status/";

public:
    pad_mqtt(const std::string& id);

    // 左舵机控制
    json set_left_servo(const json& args) override;

    // 右舵机控制
    json set_right_servo(const json& args) override;

    // 设置左补光灯
    json set_left_lamp(const json& args) override;

    // 设置右补光灯
    json set_right_lamp(const json& args) override;
};

}; // namespace robot_device

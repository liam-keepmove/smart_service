#pragma once

#include "device.hpp"
#include <atomic>

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
    enum { RUN = 0,
           STOP = 1 };

    std::atomic_int status = STOP;

public:
    json speed_front_move(const json& args) override;

    json speed_back_move(const json& args) override;

    json location_speed_move(const json& args) override;

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
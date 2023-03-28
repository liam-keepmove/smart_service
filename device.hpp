#include "json.hpp"

namespace robot_device {


class camera {
public:
    // 检测螺栓
    json detect_bolt(json args);

    // 检测仪表
    json detect_meter(json args);

    // 检测拉杆
    json detect_pull_rod(json args);

private:
    // 截图
    json shot();
};

class action_body {
public:
    // 速度和方向
    json direct_speed_move(json args);
    // 位置模式
    json location_speed_move(json args);

private:
    json get_status();
};

class ptz {
public:
    json set_xyz(json args);

private:
    json get_status();
};

class lamp {
public:
    json set_light(json args);

private:
    json get_status();
};

} // namespace robot_device
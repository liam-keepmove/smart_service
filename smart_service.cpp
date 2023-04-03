#include "device_mqtt.hpp"
#include "json.hpp"
#include "robot.hpp"
#include "task.hpp"
#include <fmt/core.h>
using fmt::print;
using fmt::println;

json active1 = json::parse(R"(
    {
        "no": 1,
        "device_code": 1,
        "active_code": 1,
        "active_args": "{\"location\": 50, \"speed\": 50}"
    }
)");

json active2 = json::parse(R"(
    {
        "no": 2,
        "device_code": 3,
        "active_code": 1,
        "active_args": "{\"rect\": [132,123,31,11], \"name\": \"指针仪表\"}"
    }
)");

json active3 = json::parse(R"(
    {
        "no": 3,
        "device_code": 1,
        "active_code": 2,
        "active_args": "{\"direction\": \"back\", \"speed\": 50}"
    }
)");

json task_json = json::parse(R"(
{
    "id": "999666",
    "type": 3,
    "priority": 1,
    "remark": "快速巡检",
    "tag": "",
    "action_list": [
        {
            "no": 2,
            "device_code": 1,
            "active_code": 2, 
            "active_args": "{\"direction\": \"back\", \"speed\": 50}",
            "remark": "再以方向模式回来,限位器会让其自行停止",
            "tag": ""
        },
        {
            "no": 1,
            "device_code": 1,
            "active_code": 1,
            "active_args": "{\"location\": 200, \"speed\": 50}",
            "remark": "直接以位置模式去终点",
            "tag": ""
        }
    ]
}
)");

int main() {
    robot mapad;
    mapad.device[1] = new robot_device::action_body_mqtt();
    mapad.device[2] = new robot_device::ptz_mqtt();
    mapad.device[3] = new robot_device::camera_mqtt();
    try {
        task check(task_json);
        check.assign_to(&mapad);
        check.run();
    } catch (json::parse_error& ex) {
        println("parse error:{}", ex.what());
    }
    for (auto& [number, device] : mapad.device) {
        delete device;
    }
}
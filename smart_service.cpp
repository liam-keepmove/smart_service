#include "device_mqtt.hpp"
#include "json.hpp"
#include "robot.hpp"
#include "task.hpp"

json active1 = json::parse(R"(
    {
        "device_number": 3,
        "active_number": 1,
        "active_args": "{\"location\": 50, \"speed\": 50}"
    }
)");

json active2 = json::parse(R"(
    {
        "device_number": 2,
        "active_number": 1,
        "active_args": "{\"rect\": [132,123,31,11], \"name\": \"指针仪表\"}"
    }
)");

json active3 = json::parse(R"(
    {
        "device_number": 1,
        "active_number": 1,
        "active_args": "{\"direction\": \"back\", \"speed\": 50}"
    }
)");

int main() {
    robot mapad;
    mapad.device[1] = new robot_device::action_body_mqtt();
    mapad.device[2] = new robot_device::ptz_mqtt();
    mapad.device[3] = new robot_device::camera_mqtt();
    task check(&mapad);
    check.active_list.push_back(active1);
    check.active_list.push_back(active2);
    check.active_list.push_back(active3);
    check.run();
}
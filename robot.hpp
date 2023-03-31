#pragma once
#include "device.hpp"
#include <functional>
#include <map>

class robot {
public:
    //    robot_device::action_body_mqtt body;
    //    robot_device::ptz_mqtt ptz;
    //    robot_device::camera_mqtt ptz_camera;
    //    robot_device::lamp_mqtt lamp;
    //    std::map<int, std::function<json(const json&)>> control{
    //        {1, std::bind(&decltype(body)::location_speed_move, &body, std::placeholders::_1)},
    //        {2, std::bind(&decltype(body)::direct_speed_move, &body, std::placeholders::_1)},
    //        {3, std::bind(&decltype(ptz)::set_xyz, &ptz, std::placeholders::_1)},
    //        {4, std::bind(&decltype(ptz_camera)::detect_bolt, &ptz_camera, std::placeholders::_1)},
    //        {5, std::bind(&decltype(ptz_camera)::detect_meter, &ptz_camera, std::placeholders::_1)},
    //        {6, std::bind(&decltype(ptz_camera)::detect_pull_rod, &ptz_camera, std::placeholders::_1)},
    //    };

    std::map<int, robot_device::device*> device;
};
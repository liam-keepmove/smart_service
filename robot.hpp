#pragma once
#include "device.hpp"
#include <functional>
#include <map>
#include <memory>

class robot {
public:
    std::map<std::string, std::unique_ptr<robot_device::device>> device;

    robot() = default;
    robot(robot&&) = default;

    // 机器人所有权只能是唯一的,不能共享
    robot(const robot&) = delete;
    robot& operator=(const robot&) = delete;
};
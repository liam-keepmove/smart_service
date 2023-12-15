#pragma once
#include "device.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>

class robot {
    std::map<std::string, std::unique_ptr<robot_device::device>> device_list;

public:
    robot() = default;
    robot(robot&&) = default;
    void add_device(const std::string& device_code, std::unique_ptr<robot_device::device>&&);
    void del_device(const std::string& device_code) noexcept;
    const std::unique_ptr<robot_device::device>& get_device(const std::string& device_code) const;
    const decltype(device_list)& get_device_list() const noexcept;

    // 机器人所有权只能是唯一的,不能共享
    robot(const robot&) = delete;
    robot& operator=(const robot&) = delete;
};
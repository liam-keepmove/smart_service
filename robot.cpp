#include "robot.hpp"

void robot::add_device(const std::string& device_code, std::unique_ptr<robot_device::device>&& device) {
    device_list[device_code] = std::move(device);
}

void robot::del_device(const std::string& device_code) noexcept {
    if (device_list.find(device_code) != device_list.end())
        device_list.erase(device_code);
}

const std::unique_ptr<robot_device::device>& robot::get_device(const std::string& device_code) const {
    return device_list.at(device_code);
}

const decltype(robot::device_list)& robot::get_device_list() const noexcept {
    return device_list;
}
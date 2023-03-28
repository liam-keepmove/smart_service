#include "device.hpp"

class pad {
public:
    robot_device::action_body body;
    robot_device::ptz pantilt;
    robot_device::camera ptz_camera;
    robot_device::lamp light;
    
};
#include <mosquitto.h>
#include <spdlog/spdlog.h>
#include <thread>
#include "json.hpp"
#include "config.hpp"
#include <cassert>
using namespace std::literals;
using namespace std::chrono_literals;

config_item global_config("./config.yml");
int battery_threshold = 10;  // [1-100)百分比,低于此百分比去充电
std::string robot_battery_topic = "/Robot/Battery/" + global_config.robot_id;
std::string robot_ctrl_topic = "/SmartSer/Robot/CtrlMove/" + global_config.robot_id;
std::string charge_payload = json::parse(R"(
{
    "action": 4,
    "arg": null
}
)").dump(4);

int main(){
    assert(battery_threshold >= 1 && battery_threshold < 100);
    while(true){
        mosquitto_subscribe_callback(
        [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
            json msg_json = json::parse((char*)message->payload);
            json battery_info_array = msg_json.at("BatteryPack");
            double all_nom_cap = 0;  // 标定电池容量
            double all_rem_cap = 0;  // 剩余电池容量
            for (const auto& battery_info : battery_info_array) {
                double amp = battery_info.at("Amp").template get<double>();
                if(amp > 0){  //有充电电流,正在充电了
                    spdlog::info("有充电电流,所以不需要再发充电指令");
                    return 1;
                }
                all_nom_cap += battery_info.at("NomCap").template get<double>(); 
                all_rem_cap += battery_info.at("RemCap").template get<double>(); 
            }
            int battery_percentage = (all_rem_cap / all_nom_cap) * 100;
            if (battery_percentage <= battery_threshold) {
                mosquitto_publish(mosq, nullptr, robot_ctrl_topic.c_str(), charge_payload.size(), charge_payload.c_str(), 2, false);
                spdlog::info("当前电量为{}%,小于等于{}%,且没有充电电流,发送充电指令让机器人去充电", battery_percentage, battery_threshold);
            } else {
                spdlog::info("当前电量为{}%,大于{}%,不发送充电指令", battery_percentage, battery_threshold);
            }
            return 1;
        },
        nullptr, robot_battery_topic.c_str(), 0, global_config.broker_ip.c_str(), global_config.broker_port, "battery_moniter", 10, true, global_config.broker_username.c_str(), global_config.broker_password.c_str(), nullptr, nullptr);
        std::this_thread::sleep_for(5min);
    }
}

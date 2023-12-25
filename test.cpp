#include <spdlog/spdlog.h>
#include <mosquitto.h>
#include <string>
#include <cstdlib>

struct config{
    std::string broker_ip = "127.0.0.1";
    int broker_port = 1883;
    std::string broker_username = "admin";
    std::string broker_password = "abcd1234";
    int broker_keep_alive = 10;
    std::string mqtt_client_id = "test";
} global_config;

int main(){
    bool is_ten = false;
    auto subscribe_callback = [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) -> int {
        auto& is_ten = *(bool*)obj;
        is_ten = atoi((char*)message->payload) == 10;
        spdlog::info("{} is ten? {}", (char*)message->payload, is_ten);
        return is_ten ? 1 : 0; // mosquitto接口规定:返回1则断开连接,非1则保留连接,尊重此接口规定
    };
    mosquitto_subscribe_callback(subscribe_callback, &is_ten, "/test", 0, global_config.broker_ip.c_str(), global_config.broker_port, "get_battery", 10, true, global_config.broker_username.c_str(), global_config.broker_password.c_str(), nullptr, nullptr);
    while(!is_ten){
        
    }
    spdlog::info("process exit.");

}

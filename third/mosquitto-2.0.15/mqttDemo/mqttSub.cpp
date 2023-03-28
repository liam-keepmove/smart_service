#include <iostream>
#include <mosquitto.h>
using std::cout;
using std::cerr;
using std::endl;
using std::string;

const char* ADDRESS = "49.233.80.153"; // MQTT broker的地址
const int PORT = 9999; // MQTT broker的端口
const int KEEP_ALIVE = 60; // 保持连接的秒数
const int QOS = 1; // MQTT服务质量

void on_message(mosquitto* mosq, void* tag, const mosquitto_message* msg){
    cout << "Received message:"  << (char *)msg->payload << endl;
    //TODO maybe memory free
}

int main(int argc, char* argv[]) {
    if(argc!=2){
        cerr << "<usage>%s subscribed_topic" << endl;
        return -1;
    }
    int rc=0;
    string topic=argv[1];

    mosquitto_lib_init();
    mosquitto* mosq = mosquitto_new(nullptr, true, nullptr);
    if (mosq==nullptr) {
        cerr << "Failed to create mosquitto object:"<< mosquitto_strerror(rc)  << endl;
        rc=-1;
        goto end;
    }

    rc = mosquitto_connect(mosq, ADDRESS, PORT, KEEP_ALIVE);
    if(rc != MOSQ_ERR_SUCCESS){
        cerr << "Failed to connect mosquitto broker:"<< mosquitto_strerror(rc)  << endl;
        rc=-1;
        goto end;
    }

    rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), QOS);
    if(rc != MOSQ_ERR_SUCCESS){
        cerr << "Failed to subscrib topic" << mosquitto_strerror(rc)  << endl;
        rc=-1;
        goto end;
    }

    mosquitto_message_callback_set(mosq,on_message);


    cerr<<"start subscribing:"<<topic<<endl;
    mosquitto_loop_forever(mosq, -1, 1);

end:if(mosq!=nullptr)
        mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return rc;
}


#include <iostream>
#include <mosquitto.h>
using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::string;

const char* ADDRESS = "49.233.80.153"; // MQTT broker的地址
const int PORT = 9999; // MQTT broker的端口
const int KEEP_ALIVE = 60; // 保持连接的秒数
const int QOS = 1; // MQTT服务质量

int main() {
	int rc=0;
	string topic;
	string payload;

	mosquitto_lib_init();
	mosquitto* mosq = mosquitto_new(nullptr, true, nullptr);
	if (mosq==nullptr) {
		cerr << "Failed to create mosquitto object" << endl;
		rc=-1;
		goto end;
	}

	rc=mosquitto_connect(mosq, ADDRESS, PORT, KEEP_ALIVE);
	if(rc!=MOSQ_ERR_SUCCESS){
		cerr << "Failed to connect mosquitto broker" << endl;
		rc=-1;
		goto end;
	}

	while(true){  //空主题或空负载则退出.
		cout << "topic:";
		std::getline(cin,topic);
		if(topic.empty())
			break;

		cout << "payload:";
		std::getline(cin,payload);
		if(payload.empty())
			break;

		rc = mosquitto_publish(mosq, nullptr, topic.c_str(), payload.size(), payload.c_str(), QOS, false);
		if(rc!=MOSQ_ERR_SUCCESS){
			cerr << "Failed to publish to MQTT broker: " << mosquitto_strerror(rc) << endl;
			goto end;
		}else
			cout << "message published" << endl;
	}

end:if(mosq!=nullptr)
		mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return rc;
}


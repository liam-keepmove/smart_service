#include <iostream>
#include <sstream>
#include "cjson/cJSON.h"
using std::string;

const char* ctrlMessage = R"(
{
    "frameId": 1, 
    "statusLight": 3, 
    "targetSpeed": 20, 
    "targetPosition": 0, 
    "motionMode": 0, 
    "camLight_1": 50, 
    "camAngle_1": 0, 
    "camSpeed_1": 50, 
    "camLight_2": 50, 
    "camAngle_2": 0, 
    "camSpeed_2": 50
}
)";

struct ctrlItem{
    int frameId = 0; 
    int statusLight = 0; 
    int targetSpeed = 0; 
    int targetPosition = 0; 
    int motionMode = 0; 
    int camLight_1 = 0; 
    int camAngle_1 = 0; 
    int camSpeed_1 = 0; 
    int camLight_2 = 0; 
    int camAngle_2 = 0; 
    int camSpeed_2 = 0;

    //jsonStr必须符合格式
    ctrlItem(const string jsonStr){
        cJSON* cjsonObj = cJSON_Parse(jsonStr.c_str());
        if(cjsonObj == nullptr)
            throw string("Parsing the json string failed.");

        frameId = cJSON_GetObjectItem(cjsonObj, "frameId")->valueint;
        statusLight = cJSON_GetObjectItem(cjsonObj, "statusLight")->valueint;; 
        targetSpeed = cJSON_GetObjectItem(cjsonObj, "targetSpeed")->valueint;; 
        targetPosition = cJSON_GetObjectItem(cjsonObj, "targetPosition")->valueint;; 
        motionMode = cJSON_GetObjectItem(cjsonObj, "motionMode")->valueint;; 
        camLight_1 = cJSON_GetObjectItem(cjsonObj, "camLight_1")->valueint;; 
        camAngle_1 = cJSON_GetObjectItem(cjsonObj, "camAngle_1")->valueint;; 
        camSpeed_1 = cJSON_GetObjectItem(cjsonObj, "camSpeed_1")->valueint;; 
        camLight_2 = cJSON_GetObjectItem(cjsonObj, "camLight_2")->valueint;; 
        camAngle_2 = cJSON_GetObjectItem(cjsonObj, "camAngle_2")->valueint;; 
        camSpeed_2 = cJSON_GetObjectItem(cjsonObj, "camSpeed_2")->valueint;;

        cJSON_Delete(cjsonObj);
        cjsonObj=nullptr;
    }

    string toString(){
        std::stringstream ss;
        ss << "{\n" 
        << "\t" << "frameId: " << frameId << ",\n"
        << "\t" << "statusLight: " << statusLight << ",\n" 
        << "\t" << "targetSpeed: " <<targetSpeed << ",\n"
        << "\t" << "targetPosition: " << targetPosition << ",\n"
        << "\t" << "motionMode: " << motionMode<< ",\n"
        << "\t" << "camLight_1: " << camLight_1<< ",\n"
        << "\t" << "camAngle_1: " << camAngle_1<< ",\n"
        << "\t" << "camSpeed_1: " << camSpeed_1<< ",\n"
        << "\t" << "camLight_2: " << camLight_2<< ",\n"
        << "\t" << "camAngle_2: " << camAngle_2<< ",\n"
        << "\t" << "camSpeed_2: " << camSpeed_2 
        << "\n}";
        return ss.str();
    } 
};

int main(){
    ctrlItem ctrlField(ctrlMessage);
    std::cout << ctrlField.toString() << std::endl;
}

.PHONY: clean all
PKG += -I./third/ -L./third/ -I./third/cppcodec-0.2/ -L./third/cppcodec-0.2/ -I./third/mosquitto-2.0.15/include -L./third/mosquitto-2.0.15/lib -L./third/mosquitto-2.0.15/lib/cpp -lmosquitto
PKG += `pkg-config --cflags --libs libcurl fmt opencv4`
FLAGS += -g -std=c++17

smart_service.out:smart_service.cpp task.o device_mqtt.o json.hpp
	g++ ${FLAGS} smart_service.cpp task.o device_mqtt.o ${PKG} -o smart_service.out -lfmt -lpthread

test.out:image_detect.o test.cpp
	g++ ${FLAGS} ${PKG} test.cpp image_detect.o -o test.out

task.o:task.hpp task.cpp robot.hpp json.hpp
	g++ ${FLAGS} -c task.cpp -latomic

device_mqtt.o:device_mqtt.hpp device_mqtt.cpp device.hpp
	g++ ${FLAGS} -c device_mqtt.cpp ${PKG} -lfmt -latomic

image_detect.o:image_detect.hpp image_detect.cpp
	g++ ${FLAGS} -c image_detect.cpp ${PKG}

clean:
	rm -rf test.out image_detect.o task.o device_mqtt.o smart_service.out

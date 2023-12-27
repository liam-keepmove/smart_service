.PHONY: clean all install powerstart unpowerstart

FLAGS += -g -std=c++17
SYSTEMD_SERVICE_NAME=hljb_smart_service.service
PKG += -I./third/cppcodec-0.2/ -L./third/cppcodec-0.2/ 
PKG += -I./third/mosquitto-2.0.15/include -L./third/mosquitto-2.0.15/lib -lmosquitto
PKG += -DSPDLOG_COMPILED_LIB -I./third/spdlog-1.12.0/include/ -L./third/spdlog-1.12.0/build/ -lspdlog -lpthread
PKG += -I./third/curl-8.4.0/include/ -L./third/curl-8.4.0/lib/.libs/ -lcurl
PKG += `pkg-config --cflags --libs opencv4 yaml-cpp`
RPATH += -Wl,-rpath,'$$ORIGIN'/:'$$ORIGIN'/lib/:'$$ORIGIN'/third/curl-8.4.0/lib/.libs/:'$$ORIGIN'/third/mosquitto-2.0.15/lib/:'$$ORIGIN'/third/spdlog-1.12.0/build/

smart_service.out: smart_service.o task.o timed_task.o robot.o device_mqtt.o image_detect.o
	g++ ${FLAGS} task.o timed_task.o robot.o device_mqtt.o image_detect.o smart_service.o ${PKG} -o smart_service.out ${RPATH}

smart_service.o: smart_service.cpp ThreadSafeQueue.hpp device_mqtt.hpp device.hpp json.hpp misc.hpp robot.hpp task.hpp timed_task.hpp config.hpp
	g++ ${FLAGS} -c smart_service.cpp ${PKG}

test.out: image_detect.o test.cpp
	g++ ${FLAGS} ${PKG} test.cpp image_detect.o -o test.out

task.o: task.cpp task.hpp robot.hpp device.hpp json.hpp
	g++ ${FLAGS} -c task.cpp -latomic

timed_task.o: timed_task.cpp timed_task.hpp json.hpp misc.hpp config.hpp
	g++ ${FLAGS} -c timed_task.cpp

robot.o: robot.cpp robot.hpp device.hpp json.hpp
	g++ ${FLAGS} -c robot.cpp

device_mqtt.o: device_mqtt.cpp device_mqtt.hpp device.hpp json.hpp misc.hpp config.hpp
	g++ ${FLAGS} -c device_mqtt.cpp ${PKG} -latomic

image_detect.o: image_detect.cpp image_detect.hpp json.hpp misc.hpp
	g++ ${FLAGS} -c image_detect.cpp ${PKG}

battery_moniter.out: battery_moniter.cpp json.hpp config.hpp
	g++ ${FLAGS} battery_moniter.cpp -o battery_moniter.out ${PKG} ${RPATH}

powerstart:
	@echo "Change the script parameter \"WorkingDirectory=\" to the current directory"
	@sed -i "s\WorkingDirectory=.*\WorkingDirectory=`pwd`\g" $(SYSTEMD_SERVICE_NAME)
	cp $(SYSTEMD_SERVICE_NAME) /etc/systemd/system
	systemctl enable $(SYSTEMD_SERVICE_NAME)
	systemctl daemon-reload
	@echo "-------------The boot-up service was successfully installed--------------"
	@echo "Enter \"systemctl start $(SYSTEMD_SERVICE_NAME)\" to start the service."
	@echo "See README.me for details."

unpowerstart:
	systemctl stop $(SYSTEMD_SERVICE_NAME)
	systemctl disable $(SYSTEMD_SERVICE_NAME)
	systemctl daemon-reload
	@echo "The boot-up service has stopped"

clean:
	rm -rf task.o timed_task.o robot.o device_mqtt.o image_detect.o smart_service.o smart_service.out

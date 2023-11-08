.PHONY: clean all
PKG += `pkg-config --cflags --libs libcurl fmt opencv4`
PKG += -I./third/ -L./third/ -I./third/cppcodec-0.2/ -L./third/cppcodec-0.2/
FLAGS += -g -std=c++17

test.out:image_detect.o test.cpp
	g++ ${FLAGS} test.cpp image_detect.o ${PKG} -o test.out

image_detect.o:image_detect.hpp image_detect.cpp
	g++ ${FLAGS} -c image_detect.cpp ${PKG}

clean:
	rm -rf test.out image_detect.o

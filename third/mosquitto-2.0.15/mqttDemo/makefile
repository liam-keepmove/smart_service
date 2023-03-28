pub:mqttPub.cpp
	g++ -std=c++11 -I../include -L../lib mqttPub.cpp -o pub.out -lmosquitto 
sub:mqttSub.cpp
	g++ -std=c++11 -I../include -L../lib mqttSub.cpp -o sub.out -lmosquitto 

decode:str2obj.cpp
	g++ -std=c++11 str2obj.cpp -o str2obj.out -lcjson

encode:obj2str.cpp
	g++ -std=c++11 obj2str.cpp -o obj2str.out -lcjson

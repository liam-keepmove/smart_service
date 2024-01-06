### 服务功能
1. 后端通过协议发送任务给此程序,此程序控制机器人执行任务并返回结果给后端
2. 电量低于10%时,自动返回充电桩充电

### 服务依赖库
NOTE: 一些服务依赖库已经编译好(linux-arm64)并放置在third目录中了,程序启动会去那儿链接
|library|version|way to install|
|-----|-----|-----|
|opencv|4.2|apt|
|libcurl|8.4.0|compile|
|spdlog|1.12.0|compile|
|cppcodec|0.2|compile|
|mosquitto|2.0.15|compile|
|yaml-cpp|0.8|compile|

### 服务依赖程序
1. mqttx: 一个mqtt客户端程序,可以给broker发送mqtt消息.
2. emqx: 一个mqtt broker,在开启本服务前需要先开启emqx服务,否则本服务无法正常工作.

### 使用make安装/卸载服务
1. 编译:make,如果你是ubuntu20.04-arm64平台,那么可以跳过这一步,因为有一个已经编译好的程序了
2. 安装开机自启服务:make powerstart
3. 取消开机自启服务:make unpowerstart
4. 清理临时文件:make clean

### 使用systemd管理该服务
1. 开启服务:systemctl start hljb_smart_service.service
2. 关闭服务:systemctl stop hljb_smart_service.service
3. 重启服务:systemctl restart hljb_smart_service.service
4. 检查服务:systemctl status hljb_smart_service.service
5. 服务开机自启:systemctl enable hljb_smart_service.service
6. 禁用开机自启:systemctl disable hljb_smart_service.service
7. 查看服务是否开机自启动:systemctl is-enabled hljb_smart_service.service

### 配置文件编辑规则
1. 配置文件为yaml格式,文件名为config.yml,放置在程序当前目录下
2. robot_id: 机器人行动本体的id,控制前进后退等动作时需要用到,必须存在
3. modules: 机器人上的设备模块,例如云台,可以不存在

### 日志相关操作
日志保存在程序当前目录下的logs目录中,日志文件名为smart_service.log

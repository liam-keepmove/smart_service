### 服务功能
上连接后端,下连接机器人

### 服务依赖库
NOTE: 一些服务依赖库已经编译好(linux-arm64)并放置在third目录中了,程序启动会去那儿链接
|library|version|way to install|
|-----|-----|-----|
|opencv|4.2|apt|
|libcurl|8.4.0|compile|
|spdlog|1.12.0|compile|
|cppcodec|0.2|compile|
|mosquitto|2.0.15|compile|

### 服务依赖程序
1. emqx broker

### 使用make安装/卸载服务
1. 编译:make
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


### 日志相关操作
1. 实时查看日志:journalctl -u hljbGateway.service -f
2. journalctl默认将日志放置在内存中,要想持久保存日志,请将/etc/systemd/journald.conf中的Storage值赋为persistent
3. 日志默认持久化路径是:/var/log/journal/
4. journalctl查看日志依赖正确的时间设置,请务必开启NTP服务,否则日志顺序可能不正确


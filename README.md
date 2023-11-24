### 服务功能

### 服务依赖库
|library|version|
|-----|-----|
|nlohmann/json|3.11.2|
|libcurl|8.4.0|
|fmt|10.1.1|
|spdlog|1.12.0|

### 使用make安装/卸载服务
1. 编译生成目标文件:make
2. 安装服务(默认开机自启):make install 
3. 清理中间文件:make clean
4. 卸载服务:make uninstall
5. 若要重新安装服务，请先卸载服务

### 使用systemd管理该服务
1. 开启服务:systemctl start hljbGateway.service
2. 关闭服务:systemctl stop hljbGateway.service
3. 重启服务:systemctl restart hljbGateway.service
4. 检查服务:systemctl status hljbGateway.service
5. 服务开机自启:systemctl enable hljbGateway.service
6. 禁用开机自启:systemctl disable hljbGateway.service
7. 查看服务是否开机自启动:systemctl is-enabled hljbGateway.service

### 日志相关操作
1. 实时查看日志:journalctl -u hljbGateway.service -f
2. journalctl默认将日志放置在内存中,要想持久保存日志,请将/etc/systemd/journald.conf中的Storage值赋为persistent
3. 日志默认持久化路径是:/var/log/journal/
4. journalctl查看日志依赖正确的时间设置,请务必开启NTP服务,否则日志顺序可能不正确

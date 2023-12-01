#pragma once

#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define THROW_RUNTIME_ERROR(WHAT) \
    throw std::runtime_error(std::string(__FILE__ ":" + std::to_string(__LINE__) + "-->" + __PRETTY_FUNCTION__ + ":") + (WHAT))

#define ARRAY_SIZE(ARR) (*(&ARR + 1) - ARR)
#define LOG() printf("%s:%d-->%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)

/**
 * @brief  禁止重开
 * @note   原理:若pid文件存在,则判断实际进程列表中此pid进程是否存在,若此pid进程存在则表明程序正在执行中,若此pid进程不存在,则创建pid文件后直接运行.
 *         守护进程请在真正起作用的进程中调用
 * @retval None
 */
class NoReopen {
private:
    std::string pidFilePath; // pid文件全路径

public:
    NoReopen(const char* pidFilePath)
        : pidFilePath(pidFilePath) {
        char pidFileContent[255] = {0};
        // 检查PID文件是否存在
        int pidfd = open(pidFilePath, O_RDONLY);
        if (pidfd > 0) {
            // 如果PID文件存在，则读取其中的PID
            if (read(pidfd, pidFileContent, ARRAY_SIZE(pidFileContent)) < 0) {
                fprintf(stderr, "read function error\n");
                exit(1);
            }

            pid_t pid = atoi(pidFileContent);

            // 检查PID是否为有效进程
            if (kill(pid, 0) == 0) { // PID文件中的pid对应的进程存在,则退出
                fprintf(stderr, "The program is already running. Process ID is %d.\n", pid);
                exit(1);
            } else {
                // 如果PID为无效进程，则删除PID文件
                remove(pidFilePath);
            }
        }
        pidfd = open(pidFilePath, O_CREAT | O_EXCL | O_WRONLY, 0666);
        if (pidfd < 0) {
            fprintf(stderr, "Unable to create pid file,Maybe root was not given permission.\n");
            exit(1);
        }

        sprintf(pidFileContent, "%d", getpid());
        if (write(pidfd, pidFileContent, strlen(pidFileContent)) <= 0) {
            fprintf(stderr, "write function error\n");
            exit(1);
        }
        close(pidfd);
    }

    ~NoReopen() {
        remove(pidFilePath.c_str());
    }
};

std::vector<std::string> split(const std::string& s, const std::string& d) {
    if (s.empty())
        return {};
    std::vector<std::string> res;
    std::string strs = s + d;
    size_t pos = strs.find(d);
    size_t len = strs.size();
    while (pos != std::string::npos) {
        std::string x = strs.substr(0, pos);
        if (x != "")
            res.push_back(x);
        strs = strs.substr(pos + d.size(), len);
        pos = strs.find(d);
    }
    return res;
}

#pragma once

#include "robot.hpp"
#include <atomic>

class task {
private:
    // 即时任务状态 -1急停;0任务暂停;1任务待执行;2动作执行中;3动作执行结果;4任务结束;6任务取消;7机器异常;8指令异常
    enum { PAUSE = 0,
           PEND_EXEC = 1,
           EXECING = 2,
           END = 4,
           CANCEL = 6,
    };

    std::atomic_int status = PEND_EXEC;
    robot* qirobot = nullptr;

public:
    std::vector<json> active_list;
    task(robot*);
    int get_status();
    void run();
    void pause();
    void resume();
    void cancel();
};
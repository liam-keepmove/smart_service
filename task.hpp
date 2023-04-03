#pragma once

#include "robot.hpp"
#include <atomic>

class task {
private:
    // 即时任务状态 -1急停;0任务暂停;1任务待执行;2任务执行中;3动作执行结果;4任务结束;6任务取消;7机器异常;8指令异常
    enum { PAUSE = 0,
           PEND_EXEC = 1,
           EXECING = 2,
           END = 4,
           CANCEL = 6,
           EXEC_RESULT = 3,
    };

    // 任务状态,只可能有:任务待执行,任务暂停,任务执行中,任务结束,任务取消
    std::atomic_int status = PEND_EXEC;

    robot* qirobot = nullptr;

    json generate_feedback(int active_no, int status, std::string result = "", std::string remark = "");

public:
    struct action {
        int no;
        int device_code;
        int active_code;
        json active_args;
        std::string remark;
        std::string tag;
        friend void from_json(const json& j, action& a);
    };

    // 任务优先级
    int priority = 0;

    // 任务执行到第几步了
    int active_no = 0;

    // 任务备注
    std::string remark;

    // 任务id
    std::string id;

    // 任务执行的次数,不论任务执行结果如何,开始执行即算一次,
    int executed_count = 0;

    // 冗余字段
    std::string tag;

    // 任务动作列表
    std::vector<action> action_list;

    task(robot*, std::string id, int executed_count);

    task(const json& task_json);

    task();

    void assign_to(robot*);

    int get_status();

    void run();

    void pause();

    void resume();

    void cancel();

    friend void from_json(const json& j, task& t);
};
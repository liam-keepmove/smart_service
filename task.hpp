#pragma once

#include "robot.hpp"
#include <atomic>

class task {
private:
    // 任务控制:开始(动作执行态),暂停(任务暂停态),恢复(动作执行态),取消(任务结束态)
    // 任务状态反馈:0任务暂停中;1任务待执行;2动作执行中;4任务已正常结束;6任务已取消
    // 其他消息反馈:7机器异常;8指令异常
    enum { PAUSE = 0,
           PEND_EXEC = 1,
           EXECING = 2,
           EXEC_RESULT = 3,
           END = 4,
           CANCEL = 6,
    };

    // 任务状态,只可能有:任务待执行,任务暂停,任务执行中,任务结束,任务取消
    std::atomic_int status = END;

    using task_step_callback = std::function<void(json)>;

    json generate_feedback(int active_no, int status, const std::string& result = "", const std::string& remark = "", const std::string& tag = "");

public:
    struct action {
        int no;
        std::string device_code;
        std::string active_code;
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

    task(const json& task_json);

    task();

    task& operator=(const task& t);

    int get_status();

    void run(robot&);

    void pause();

    void resume();

    void cancel();

    bool is_over();

    task_step_callback task_will_start_callback = nullptr;
    task_step_callback action_start_callback = nullptr;
    task_step_callback action_result_callback = nullptr;
    task_step_callback pause_callback = nullptr;
    task_step_callback cancel_callback = nullptr;
    task_step_callback over_callback = nullptr;

    friend void from_json(const json& j, task& t);
};
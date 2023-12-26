#pragma once

#include "robot.hpp"
#include <atomic>
#include <mutex>
#include <optional>

class task {
public:
    // 任务状态:任务开始,任务暂停中,任务执行中,任务结束
    // 任务主动控制:开始(任务执行态),暂停(任务暂停态),恢复(任务执行态),取消(任务结束态)
    // 任务消息反馈:1任务开始;0任务暂停中(任务暂停态);2动作执行中(任务执行态);3动作执行结果(任务执行态);4任务结束(任务结束态);6任务取消(任务结束态)
    // 其他消息反馈:7机器异常;8指令异常
    enum { INIT = -1,
           PAUSE = 0,
           START = 1,
           EXECING = 2,
           EXEC_RESULT = 3,
           END = 4,
           CANCEL = 6,
           MACHINE_EXCEPTION = 7,
           INSTRUCTION_EXCEPTION = 8,
           REQ_PAUSE,
           REQ_RESUME,
           REQ_CANCEL
    };

private:
    std::atomic_int status = INIT;
    std::atomic_int req_status = INIT;
    std::mutex control;  // 在微观层面,控制(暂停,恢复,取消)必须是串行的,不能多人同时控制,不然处理就乱了
    std::mutex run_sche; // 执行调度器在同一时刻也必须只有一个,也就是run函数只能同时运行一次!
    // 要执行当前任务的机器人,只能在run函数中赋值(run接受引用),1.保证在任务执行过程中,机器人不会被删除 2.暂停,取消函数需要让机器人立刻停止
    robot* bot = nullptr;

    using task_step_callback = std::function<void(json)>;
    task_step_callback empty_callback = [](json) {};

    json generate_feedback(int active_no, int status, const std::string& result, const std::optional<std::string>& remark, const std::optional<std::string>& tag);

public:
    struct action {
        int no;
        std::string device_code;
        std::string active_code;
        json active_args;
        std::optional<std::string> remark;
        std::optional<std::string> tag;
        friend void from_json(const json& j, action& a);
    };

    // 任务优先级
    int priority = 0;

    // 任务执行到第几步了
    int active_no = 0;

    // 任务id
    std::string id;

    // 最大执行次数,在定时任务转换的即时任务里面会使用这个参数决定该定时任务是否执行完毕
    int max_count = 0;

    // 任务执行的次数,不论任务执行结果如何,开始执行即算一次,
    int executed_count = 0;

    // 任务备注
    std::optional<std::string> remark;

    // 冗余字段
    std::optional<std::string> tag;

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
    bool is_empty();

    task_step_callback task_will_start_callback = empty_callback;
    task_step_callback action_start_callback = empty_callback;
    task_step_callback action_result_callback = empty_callback;
    task_step_callback pause_callback = empty_callback;
    task_step_callback cancel_callback = empty_callback;
    task_step_callback over_callback = empty_callback;

    friend void from_json(const json& j, task& t);
};

#include "task.hpp"
#include <chrono>
#include <fmt/core.h>
#include <stdexcept>
#include <thread>
using fmt::println;

void from_json(const json& j, task::action& a) {
    j.at("no").get_to(a.no);
    j.at("device_code").get_to(a.device_code);
    j.at("active_code").get_to(a.active_code);
    a.active_args = json::parse(j.at("active_args").template get<std::string>());
    j.at("remark").get_to(a.remark);
    j.at("tag").get_to(a.tag);
}

void from_json(const json& j, task& t) {
    j.at("id").get_to(t.id);
    j.at("priority").get_to(t.priority);
    j.at("remark").get_to(t.remark);
    j.at("tag").get_to(t.tag);
    if (j.contains("executed_count"))
        j.at("executed_count").get_to(t.executed_count);
    else
        t.executed_count = 0;

    json action_list_array = j.at("action_list");
    if (action_list_array.is_array()) {
        for (auto& action : action_list_array) {
            t.action_list.push_back(action);
        }
        std::sort(t.action_list.begin(), t.action_list.end(), [](const task::action& left, const task::action& right) {
            // 动作列表以No标号为顺序,升序排列
            return left.no < right.no;
        });
    } else {
        throw std::runtime_error("field with \"action_list\" must be array of json");
    }
}

task::task(const json& task_json) {
    from_json(task_json, *this);
}

// 默认空任务是END态的.
task::task() = default;

// 更新任务
task& task::operator=(const task& t) {
    this->priority = t.priority;
    this->active_no = t.active_no;
    this->remark = t.remark;
    this->id = t.id;
    this->executed_count = t.executed_count;
    this->tag = t.tag;
    this->action_list = t.action_list;
    this->status = t.status.load();
    this->task_will_start_callback = t.task_will_start_callback;
    this->action_start_callback = t.action_start_callback;
    this->action_result_callback = t.action_result_callback;
    this->pause_callback = t.pause_callback;
    this->cancel_callback = t.cancel_callback;
    this->over_callback = t.over_callback;
    return *this;
}

int task::get_status() {
    return status;
}

json task::generate_feedback(int active_no, int status, const std::string& result, const std::string& remark, const std::string& tag) {
    static json feedback;
    feedback["task_id"] = id;
    feedback["count"] = executed_count;
    feedback["action_no"] = active_no;
    feedback["time"] = time(nullptr);
    feedback["status"] = status;
    feedback["result"] = result;
    feedback["remark"] = remark;
    feedback["tag"] = tag;
    return feedback;
}

void task::run(robot& bot) {
    using namespace std::chrono_literals;
#define XX(callback, json_obj) \
    if (callback != nullptr)   \
        callback(json_obj);
    ++executed_count;
    XX(task_will_start_callback, generate_feedback(0, PEND_EXEC, "", "The task will start.")); // 任务将要开始的反馈
    status = EXECING;
    active_no = 0;
    for (const auto& active : action_list) {
        // 注意,这里需要记得status可以随时被外部修改
        if (status == CANCEL) {
            XX(cancel_callback, generate_feedback(active_no, CANCEL)); // 任务取消反馈
            break;
        }
        if (status == PAUSE) {
            XX(pause_callback, generate_feedback(active_no, PAUSE)); // 任务暂停反馈
            while (status == PAUSE) {
                std::this_thread::sleep_for(200ms);
            }
        }
        if (status == EXECING) {
            active_no = active.no;
            XX(action_start_callback, generate_feedback(active_no, EXECING, "", "The action will exec")); // 任务执行中反馈
            json result = bot.device.at(active.device_code)->get_action(active.active_code)(active.active_args);
            XX(action_result_callback, generate_feedback(active_no, EXEC_RESULT, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
        }
    }
    // 任务结束回调
    XX(over_callback, generate_feedback(active_no, END, "The task is over.", remark, tag));
    status = END;
#undef XX
}

void task::pause() {
    if (status == PEND_EXEC || status == EXECING) {
        status = PAUSE;
        // 停止所有动作,让任务执行循环从EXECING状态的设备阻塞状态返回
        // for (auto& [number, device] : bot.device) {
        //     device->stop();
        // }
    }
}

void task::resume() {
    if (status == PAUSE)
        status = EXECING;
}

void task::cancel() {
    if (status != CANCEL && status != END) {
        status = CANCEL;
        // 停止所有动作,让任务执行循环从EXECING状态的设备阻塞状态返回
        // for (auto& [number, device] : bot.device) {
        //     device->stop();
        // }
    }
}

bool task::is_over() {
    if (status == END)
        return true;
    else
        return false;
}

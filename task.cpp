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

task::task(robot* bot, std::string id, int executed_count)
    : qirobot(bot), id(id), executed_count(executed_count) {
    if (qirobot == nullptr)
        throw std::runtime_error("pointer can't nullptr!");
}

task::task() = default;

void task::assign_to(robot* bot) {
    qirobot = bot;
    if (qirobot == nullptr)
        throw std::runtime_error("pointer can't nullptr!");
}

int task::get_status() {
    return status;
}

json task::generate_feedback(int active_no, int status, std::string result, std::string remark) {
    json feedback;
    feedback["type"] = 8;
    feedback["task_id"] = id;
    feedback["count"] = executed_count;
    feedback["action_no"] = active_no;
    feedback["time"] = time(nullptr);
    feedback["status"] = status;
    feedback["result"] = result;
    feedback["remark"] = remark;
    feedback["tag"] = "";
    return feedback;
}

void task::run() {
    using namespace std::chrono_literals;
    status = EXECING;
    active_no = 0;
    ++executed_count;
    for (const auto& active : action_list) {
        if (status == PAUSE) {
            println("{}", generate_feedback(active_no, PAUSE).dump(4)); // 任务暂停反馈
            while (status == PAUSE) {
                std::this_thread::sleep_for(200ms);
            }
        }
        if (status == CANCEL) {
            println("{}", generate_feedback(active_no, CANCEL).dump(4)); // 任务取消反馈
            return;
        } else if (status == EXECING) {
            active_no = active.no;
            println("{}", generate_feedback(active_no, EXECING, "动作执行中:" + active.remark).dump(4)); // 任务执行中反馈
            json result = qirobot->device.at(active.device_code)->get_action(active.active_code)(active.active_args);
            println("{}", generate_feedback(active_no, EXEC_RESULT, result.dump(0), "动作执行完成:" + active.remark).dump(4)); // 任务执行结果反馈
        }
    }
    println("{}", generate_feedback(active_no, END).dump(4)); // 任务结束反馈
    status = END;
}

void task::pause() {
    if (status == PEND_EXEC || status == EXECING) {
        status = PAUSE;
        // 停止所有动作
        for (auto& [number, device] : qirobot->device) {
            device->stop();
        }
    }
}

void task::resume() {
    if (status == PAUSE)
        status = EXECING;
}

void task::cancel() {
    status = CANCEL; // 先让调度器停下来
    // 停止所有动作
    for (auto& [number, device] : qirobot->device) {
        device->stop();
    }
}

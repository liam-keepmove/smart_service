#include "task.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>
using namespace std::chrono_literals;

NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        j = opt.has_value() ? json(*opt) : json(nullptr);
    }

    static void from_json(const json& j, std::optional<T>& opt) {
        opt = j.is_null() ? std::nullopt : std::make_optional<T>(j.get<T>());
    }
};
NLOHMANN_JSON_NAMESPACE_END

void from_json(const json& j, task::action& a) {
    j.at("no").get_to(a.no);
    j.at("device_code").get_to(a.device_code);
    j.at("active_code").get_to(a.active_code);
    json args = j.at("active_args");
    json arg;
    if (!args.at("arg").is_null())
        arg = json::parse(args.at("arg").template get<std::string>());
    args["arg"] = arg;
    a.active_args = args;
    j.at("remark").get_to(a.remark);
    j.at("tag").get_to(a.tag);
}

void from_json(const json& j, task& t) {
    j.at("id").get_to(t.id);
    j.at("priority").get_to(t.priority);
    j.at("max_count").get_to(t.max_count);
    j.at("executed_count").get_to(t.executed_count);
    j.at("remark").get_to(t.remark);
    j.at("tag").get_to(t.tag);
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

task::task() = default;

// 更新任务
task& task::operator=(const task& t) {
    std::scoped_lock lock(run_sche); // 确保更新的时候,调度器没有运行,防止更改了调度器运行时数据
    if (this == &t) {
        return *this;
    }
    this->id = t.id;
    this->priority = t.priority;
    this->max_count = t.max_count;
    this->executed_count = t.executed_count;
    this->remark = t.remark;
    this->tag = t.tag;
    this->action_list = t.action_list;

    this->active_no = t.active_no;
    this->bot = t.bot;
    this->status = t.status.load();
    this->req_status = t.req_status.load();

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

json task::generate_feedback(int active_no, int status, const std::string& result, const std::optional<std::string>& remark, const std::optional<std::string>& tag) {
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

void task::run(robot& qibot) {
    std::scoped_lock lock(run_sche); // 同一时刻,只能有一个调度器在运行
    if (executed_count >= max_count) {
        spdlog::error("The number of tasks that can be executed is insufficient.");
        return;
    }
    this->bot = &qibot;
    using namespace std::chrono_literals;
    ++executed_count;
    task_will_start_callback(generate_feedback(0, START, "", "The task will start.", tag)); // 任务将要开始的反馈
    active_no = 0;
    status = EXECING;
    // 这里只允许更新status,在暂停,恢复,取消函数里只允许更新req_status
    for (int i = 0; i < action_list.size();) {
        // 注意,req_status可以随时被外部修改
        if (req_status == REQ_CANCEL) {
            if (status == EXECING) {
                status = CANCEL;
                cancel_callback(generate_feedback(active_no, CANCEL, "", "The task will cancel.", tag)); // 任务取消反馈
                break;
            }
        }
        if (req_status == REQ_RESUME) {
            if (status == PAUSE)
                status = EXECING;
        }
        if (req_status == REQ_PAUSE) {
            if (status == EXECING) {
                status = PAUSE;
                pause_callback(generate_feedback(active_no, PAUSE, "", "The task will pause.", tag)); // 任务暂停反馈
                while (req_status != REQ_RESUME && req_status != REQ_CANCEL) {                        // 暂停时阻塞住
                    std::this_thread::sleep_for(100ms);
                }
            }
        }
        if (status == EXECING) {
            status = EXECING;
            auto active = action_list.at(i);
            active_no = active.no;
            action_start_callback(generate_feedback(active_no, EXECING, "", "The action will exec", active.tag)); // 任务执行中反馈
            json result;
            try {
                result = bot->get_device(active.device_code)->get_action(active.active_code)(active.active_args);
                result["remark"] = "Executed successfully";
                if (result.contains("emergency_stop")) { // 是暂停导致的返回,那么此动作在恢复时,还要再执行一次,所以i不自增

                } else {
                    // 正常返回,执行下一个动作
                    action_result_callback(generate_feedback(active_no, EXEC_RESULT, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
                    ++i;
                }
            } catch (const std::out_of_range& ex) {
                spdlog::error("device_code or active_code no found");
                result["remark"] = "Executed fail, device_code or active_code no found";
                action_result_callback(generate_feedback(active_no, INSTRUCTION_EXCEPTION, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
                spdlog::info("throw info:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                break;
            } catch (const json::parse_error& ex) {
                spdlog::error("json parse error");
                result["remark"] = "Executed fail, json parse error";
                action_result_callback(generate_feedback(active_no, INSTRUCTION_EXCEPTION, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
                spdlog::info("throw info:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                break;
            } catch (const json::out_of_range& ex) {
                spdlog::error("key of json no found");
                result["remark"] = "Executed fail, key of json no found";
                action_result_callback(generate_feedback(active_no, INSTRUCTION_EXCEPTION, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
                spdlog::info("throw info:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                break;
            } catch (const json::type_error& ex) {
                spdlog::error("json type parse error");
                result["remark"] = "Executed fail, json type parse error";
                action_result_callback(generate_feedback(active_no, INSTRUCTION_EXCEPTION, result.dump(0), active.remark, active.tag)); // 任务执行结果反馈
                spdlog::info("throw info:{}:{}\n{}", __FILE__, __LINE__, ex.what());
                break;
            }
        }
    }
    status = END;
    // 任务结束回调
    over_callback(generate_feedback(active_no, END, "The task is over.", remark, tag));
#undef XX
}

void task::pause() {
    std::scoped_lock lock(control);
    if (status == EXECING) {
        req_status = REQ_PAUSE;
        // 停止所有动作,让任务执行循环从EXECING状态的设备阻塞状态返回
        for (auto& [number, device] : bot->get_device_list()) {
            device->stop();
        }
        std::this_thread::sleep_for(100ms);
        if (status != PAUSE && status != END) {
            spdlog::info("waiting for current task pause...");
            while (status != PAUSE && status != END) {
                std::this_thread::sleep_for(100ms);
            }
        }
    }
}

void task::resume() {
    std::scoped_lock lock(control);
    if (status == PAUSE) {
        req_status = EXECING;
        if (status != EXECING && status != END) {
            spdlog::info("waiting for current task resume...");
            while (status != EXECING && status != END) {
                std::this_thread::sleep_for(100ms);
            }
        }
    }
}

void task::cancel() {
    std::scoped_lock lock(control);
    if (status == EXECING || status == PAUSE) {
        req_status = REQ_CANCEL;
        // 停止所有动作,让任务执行循环从EXECING状态的设备阻塞状态返回
        for (auto& [number, device] : bot->get_device_list()) {
            device->stop();
        }
        std::this_thread::sleep_for(100ms);
        if (!is_over()) {
            spdlog::info("waiting for current task over...");
            while (!is_over()) {
                std::this_thread::sleep_for(100ms);
            }
        }
    }
}

bool task::is_empty() {
    if (status == INIT)
        return true;
    else
        return false;
}

bool task::is_over() {
    if (status == END)
        return true;
    else
        return false;
}

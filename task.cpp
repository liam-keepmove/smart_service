#include "task.hpp"
#include <fmt/core.h>
#include <stdexcept>
using fmt::println;

task::task(robot* bot)
    : qirobot(bot) {
    if (qirobot == nullptr)
        throw std::runtime_error("pointer can't nullptr!");
}

int task::get_status() {
    return status;
}

void task::run() {
    for (const auto& active : active_list) {
        int device_number = active["device_number"].template get<int>();
        int active_number = active["active_number"].template get<int>();
        json args = json::parse(active["active_args"].template get<std::string>());
        println("{}", qirobot->device.at(device_number)->get_action(active_number)(args).dump(4));
    }
}

void task::pause() {
    if (status == PEND_EXEC || status == EXECING)
        status = PAUSE;
}

void task::resume() {
    if (status == PAUSE)
        status = PEND_EXEC;
}

void task::cancel() {
    status = CANCEL;
}

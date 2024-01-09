#include "timed_task.hpp"
#include "config.hpp"
#include "json.hpp"
#include "misc.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
extern config_item global_config;

// 根据timed_task_list更新cron_file和task_file
void timed_task_set::update_file() {
    // 删除timed_task目录下的所有文件,重新生成,以保证任务文件和cron_file一致
    std::filesystem::remove_all(task_file_path);
    std::filesystem::create_directory(task_file_path);
    std::ofstream cron_file(cron_file_path);
    if (!cron_file.is_open()) {
        THROW_RUNTIME_ERROR("Unable to open cron_file:" + cron_file_path);
    }
    cron_file << "SHELL=/bin/bash" << std::endl;
    cron_file << "PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin" << std::endl;
    cron_file << "WORK_DIR=" << std::filesystem::current_path() << std::endl;

    for (const auto& task : timed_task_list) {
        std::string task_id = task.at("id").get<std::string>();
        std::string cron_expr = task.at("cron").get<std::string>();
        std::string command = fmt::format("cd ${{WORK_DIR}} && cat ./timed_task/{}.json | ./mqttx pub -V 3.1.1 --username '{}' --password '{}' -h '{}' -p {} -t '{}' -q 2 --stdin",
                                          task.at("id").template get<std::string>(), global_config.broker_username, global_config.broker_password, global_config.broker_ip, global_config.broker_port, task_recv_topic);
        cron_file << cron_expr << " root " << command << std::endl;
        std::ofstream task_file(task_file_path + task_id + ".json"); // 创建./timed_task/xxx.json
        if (!task_file.is_open()) {
            THROW_RUNTIME_ERROR("Unable to open cron_file:" + task_file_path + task_id + ".json");
        }
        task_file << task.dump(4) << std::endl;
        task_file.close();
    }
    cron_file.close();
}

// 将cron_expr存在且对应task_file也存在的item添加进timed_task_list
timed_task_set::timed_task_set(const std::string& cron_file_path, const std::string& task_recv_topic)
    : cron_file_path(cron_file_path), task_recv_topic(task_recv_topic) {
    if (!std::filesystem::exists(cron_file_path)) {
        // 若此文件不存在,新建一个空文件
        std::ofstream ofs(cron_file_path);
        ofs.close();
    }
    if (!std::filesystem::exists(task_file_path)) {
        // 若此文件夹不存在,新建一个空文件夹
        std::filesystem::create_directory(task_file_path);
    }
    std::ifstream cron_file(cron_file_path);
    if (!cron_file.is_open()) {
        THROW_RUNTIME_ERROR("Unable to open cron_file:" + cron_file_path);
    }
    std::regex cron_expr_pattern(R"((^[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+)\s+([\w]{1,32})\s+(.+)$)");
    std::smatch matches;
    std::string line;
    while (std::getline(cron_file >> std::ws, line)) {
        if (line.empty() && line[0] == '#') {
            continue;
        }
        if (std::regex_match(line, matches, cron_expr_pattern)) {
            // cron_expr存在
            std::string command = matches[3].str();
            std::regex command_pattern(R"(^.*?cat\s\./timed_task/(.*)\.json.*$)");
            if (std::regex_match(command, matches, command_pattern)) {
                std::string task_file = task_file_path + matches[1].str() + ".json";
                if (std::filesystem::exists(task_file)) {
                    // task_file也存在
                    // 添加进timed_task_list
                    std::ifstream f(task_file);
                    if (!f.is_open()) {
                        THROW_RUNTIME_ERROR("Unable to open task_file:" + task_file);
                    }
                    timed_task_list.emplace_back(json::parse(f));
                    f.close();
                }
            }
        }
    }
    cron_file.close();
    update_file();
}

void timed_task_set::add_timed_task(json task) {
    task["type"] = 3; // 3表示即时任务,将定时任务转换为即时任务存储起来,后续直接定时给自己下发即时任务来实现定时任务
    for (auto it = timed_task_list.begin(); it != timed_task_list.end(); ++it) {
        // 如果任务id相同,则更新任务,用新的任务直接替换旧的任务
        if ((*it).at("id") == task.at("id")) {
            timed_task_list.erase(it);
            break;
        }
    }
    timed_task_list.emplace_back(task); // vector是有序的,这样在cron_file最底下可以看到最近更新或添加的任务
    update_file();
}

void timed_task_set::del_timed_task(const std::string& task_id) {
    for (auto it = timed_task_list.begin(); it != timed_task_list.end(); ++it) {
        std::string id = (*it).at("id").template get<std::string>();
        if (id == task_id) {
            timed_task_list.erase(it);
            update_file();
            return;
        }
    }
}

const decltype(timed_task_set::timed_task_list)& timed_task_set::get_timed_task_list() const noexcept {
    return timed_task_list;
}

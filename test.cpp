#include "image_detect.hpp"
#include "json.hpp"
#include "misc.hpp"
#include <curl/curl.h>
#include <fmt/core.h>
#include <iostream>
#include <mosquitto.h>
#include <regex>
#include <string_view>
using fmt::print;
using fmt::println;
using json = nlohmann::ordered_json;

struct timed_task {
    std::string cron_expr;
    std::string username;
    std::string command;

    timed_task(std::string cron_expr, std::string username, std::string command)
        : cron_expr(cron_expr), username(username), command(command) {
    }

    std::string to_cron_str() const {
        return fmt::format("{cron_expr} {username} {command}", fmt::arg("cron_expr", cron_expr), fmt::arg("username", username), fmt::arg("command", command));
    }

    friend void to_json(json& j, const timed_task& t) {
        j = json{{"cron_expr", t.cron_expr}, {"username", t.username}, {"command", t.command}};
    }

    friend void from_json(const json& j, timed_task& t) {
        j.at("cron_expr").get_to(t.cron_expr);
        j.at("username").get_to(t.username);
        j.at("command").get_to(t.command);
    }
};

struct timed_task_set {
private:
    std::string cron_file_path;
    std::string timed_task_storage_directory;

public:
    std::vector<timed_task> timed_task_list;

    timed_task_set(const std::string& cron_file_path, const std::string& timed_task_storage_directory)
        : cron_file_path(cron_file_path), timed_task_storage_directory(timed_task_storage_directory) {
        std::ifstream cron_file(cron_file_path);
        if (!cron_file.is_open()) {
            THROW_RUNTIME_ERROR("Unable to open cron_file:" + cron_file_path);
        }
        std::regex pattern(R"((^[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+\s+[-*?/,a-zA-Z0-9]+)\s+([\w]{1,32})\s+(.+)$)");
        std::smatch matches;
        std::string line;
        while (std::getline(cron_file >> std::ws, line)) {
            if (line.empty() && line[0] == '#') {
                continue;
            }
            if (std::regex_match(line, matches, pattern)) {
                timed_task_list.emplace_back(matches[1].str(), matches[2].str(), matches[3].str());
            }
        }
        cron_file.close();
    }

    void update_cron_file(const std::string& update_file_path = "") {
        if (!update_file_path.empty()) {
            cron_file_path = update_file_path;
        }
        std::ofstream cron_file(cron_file_path);
        if (!cron_file.is_open()) {
            THROW_RUNTIME_ERROR("Unable to open cron_file:" + cron_file_path);
        }
        cron_file << "SHELL=/bin/sh" << std::endl;
        cron_file << "PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin" << std::endl;
        cron_file << "TIMED_TASK_PATH=" << std::filesystem::current_path() / "timed_task" << std::endl;
        for (const auto& task : timed_task_list) {
            cron_file << task.to_cron_str() << std::endl;
        }
        cron_file.close();
    }

    friend void to_json(json& j, const timed_task_set& ts) {
        json temp;
        for (const auto& task : ts.timed_task_list) {
            temp.emplace_back(task);
        }
        j["timed_task_set"] = temp;
    }

    friend void from_json(const json& j, timed_task_set& ts) {
        ts.timed_task_list.clear();
        if (j.contains("timed_task_set")) {
            json timed_task_set_array = j.at("timed_task_set");
            if (timed_task_set_array.is_array()) {
                for (const auto& task : timed_task_set_array) {
                    ts.timed_task_list.emplace_back(task);
                }
            } else {
                THROW_RUNTIME_ERROR("field with \"timed_task_set\" must be array of json");
            }
        } else {
            THROW_RUNTIME_ERROR("field with \"timed_task_set\" must exist");
        }
    }
};

int main() {
    //    std::string json_str = R"(
    //        {
    //            "Base64Img": "",
    //            "Alg": [
    //                {
    //                    "Cls": [4, 5],
    //                    "Range": 3,
    //                    "Conf": 0.8,
    //                    "Rect": [63,165,219,339],
    //                    "Name": "指针仪表",
    //                    "DeviceCode": "123456",
    //                    "ModelCode": "KeypointMeter"
    //                }
    //            ]
    //        }
    //    )";
    //    cv::Mat image = cv::imread("test.jpg");
    //    auto base64 = img_to_base64(image);
    //    auto json_config = nlohmann::json::parse(json_str);
    //    json_config["Base64Img"] = base64;
    //    auto result = detect_image(json_config);
    //    fmt::println("{}", result.dump(4));
    //    cv::imwrite("output.jpg", base64_to_img(result["Base64Img"]));
}
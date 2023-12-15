#include "json.hpp"
#include <string>
#include <string_view>
#include <vector>

struct timed_task_set {
private:
    const std::string cron_file_path;
    const std::string task_file_path = "./timed_task/";
    std::vector<json> timed_task_list;

    // 根据timed_task_list更新cron_file和task_file
    void update_file();

public:
    // 将cron_expr存在且对应task_file也存在的item组织成timed_task_list
    timed_task_set(const std::string& cron_file_path);

    // 如果任务id不同,将task直接添加到timed_task_list中,如果任务id相同,则更新task
    void add_timed_task(json task);

    // 从timed_task_list中删除task_id对应的timed_task
    void del_timed_task(const std::string& task_id);

    const decltype(timed_task_list)& get_timed_task_list() const noexcept;
};

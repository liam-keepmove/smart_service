#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue& other) {
        std::lock_guard<std::mutex> lock(other.mutex);
        queue = other.queue;
    }

    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(value);
        cond.notify_one();
    }

    /**
     * @description: 非阻塞pop,成功返回true,失败返回false
     * @param {T&} value
     * @return {*}
     */
    bool tryPop(T& value) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty())
            return false;

        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    /**
     * @description: 一直阻塞,直到有值
     * @param {T&} value
     * @return {*}
     */
    void pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this] { return !queue.empty(); });

        value = std::move(queue.front());
        queue.pop();
    }

    /**
     * @description: 一定的毫秒内pop,超时返回false,有值返回true
     * @param {T&} value
     * @param {int} milliseconds
     * @return {*}
     */
    bool popTimeout(T& value, int milliseconds) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!cond.wait_for(lock, std::chrono::milliseconds(milliseconds),
                           [this]() { return !queue.empty(); })) {
            return false;
        }
        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    /**
     * @description: 直到时间点前若有值,则返回true,否则超时返回false
     * @param {T&} value
     * @param {time_point&} timeoutValue
     * @return {*}
     */
    bool popUntil(T& value, const std::chrono::system_clock::time_point& timeoutValue) {
        std::unique_lock<std::mutex> lock(mutex);
        if (cond.wait_until(lock, timeoutValue, [this]() { return !queue.empty(); })) {
            // 没超时,超时有值也不会取
            if (std::chrono::system_clock::now() <= timeoutValue) {
                value = std::move(queue.front());
                queue.pop();
                return true;
            }
        }
        return false;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    std::condition_variable cond;
};

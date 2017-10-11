// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include "common/assert.h"

namespace Common {

class ThreadPool {
private:
    explicit ThreadPool(size_t num_threads) : num_threads(num_threads), workers(num_threads) {
        ASSERT(num_threads);
    }

public:
    static ThreadPool& GetPool() {
        static ThreadPool thread_pool(std::thread::hardware_concurrency());
        return thread_pool;
    }

    template <typename F, typename... Args>
    auto Push(F&& f, Args&&... args) {
        auto ret = workers[next_worker].Push(std::forward<F>(f), std::forward<Args>(args)...);
        next_worker = (next_worker + 1) % num_threads;
        return ret;
    }

    const size_t total_threads() const {
        return num_threads;
    }

private:
    template <typename T>
    class ThreadsafeQueue {
    private:
        const size_t capacity;
        std::vector<T> queue_storage;
        std::mutex mutex;
        std::condition_variable queue_changed;

    public:
        explicit ThreadsafeQueue(const size_t capacity) : capacity(capacity) {
            queue_storage.reserve(capacity);
        }

        void Push(const T& element) {
            std::unique_lock<std::mutex> lock(mutex);
            while (queue_storage.size() >= capacity) {
                queue_changed.wait(lock);
            }
            queue_storage.push_back(element);
            queue_changed.notify_one();
        }

        T Pop() {
            std::unique_lock<std::mutex> lock(mutex);
            while (queue_storage.empty()) {
                queue_changed.wait(lock);
            }
            T element(std::move(queue_storage.back()));
            queue_storage.pop_back();
            queue_changed.notify_one();
            return element;
        }

        void Push(T&& element) {
            std::unique_lock<std::mutex> lock(mutex);
            while (queue_storage.size() >= capacity) {
                queue_changed.wait(lock);
            }
            queue_storage.emplace_back(std::move(element));
            queue_changed.notify_one();
        }
    };

    class Worker {
    private:
        ThreadsafeQueue<std::function<void()>> queue;
        std::thread thread;
        static constexpr size_t MAX_QUEUE_CAPACITY = 100;

    public:
        Worker() : queue(MAX_QUEUE_CAPACITY), thread([this] { Loop(); }) {}

        ~Worker() {
            queue.Push(nullptr); // Exit the loop
            thread.join();
        }

        void Loop() {
            while (true) {
                std::function<void()> fn(queue.Pop());
                if (!fn) // a nullptr function is the signal to exit the loop
                    break;
                fn();
            }
        }

        template <typename F, typename... Args>
        auto Push(F&& f, Args&&... args) {
            auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            queue.Push([task] { (*task)(); });
            return task->get_future();
        }
    };

    const size_t num_threads;
    size_t next_worker = 0;
    std::vector<Worker> workers;
};

} // namespace Common

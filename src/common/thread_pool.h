// Copyright 2016 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>

#include "common/assert.h"

namespace Common {

class ThreadPool : NonCopyable {
private:
    explicit ThreadPool(unsigned int num_threads) :
        num_threads(num_threads),
        workers(num_threads) {
        ASSERT(num_threads);
    }

public:
    static ThreadPool& GetPool() {
        static ThreadPool thread_pool(std::thread::hardware_concurrency());
        return thread_pool;
    }

    void set_spinlocking(bool enable) {
        for (auto& worker : workers) {
            worker.spinlock_enabled = enable;
            if (enable) {
                std::unique_lock<std::mutex> lock(worker.mutex);
                lock.unlock();
                worker.cv.notify_one();
            }
        }
    }

    template <typename F, typename... Args>
    auto push(F&& f, Args&&... args) {
        auto ret = workers[next_worker].push(std::forward<F>(f), std::forward<Args>(args)...);
        next_worker = (next_worker + 1) % num_threads;
        return ret;
    }

    unsigned int total_threads() {
        return num_threads;
    }

private:
    class Worker {
    public:
        Worker() :
            exit_loop(false),
            spinlock_enabled(false),
            thread([this] { loop(); }) {
        }

        ~Worker() {
            exit_loop = true;
            std::unique_lock<std::mutex> lock(mutex);
            lock.unlock();
            cv.notify_one();
            thread.join();
        }

        void loop() {
            for (;;) {
                while (queue.consume_all([](const auto& f) {
                    f();
                }));
                if (spinlock_enabled)
                    continue;

                std::unique_lock<std::mutex> lock(mutex);
                if (queue.read_available())
                    continue;
                if (exit_loop)
                    break;
                cv.wait(lock);
            }
        }

        template <typename F, typename... Args>
        auto push(F&& f, Args&&... args) {
            auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );

            while (!queue.push([task]() {(*task)(); }))
                std::this_thread::yield();

            if (!spinlock_enabled.load(std::memory_order_relaxed)) {
                std::unique_lock<std::mutex> lock(mutex);
                lock.unlock();
                cv.notify_one();
            }

            return task->get_future();
        }

        bool exit_loop;
        std::atomic<bool> spinlock_enabled;
        std::mutex mutex;
        std::condition_variable cv;
        boost::lockfree::spsc_queue<std::function<void()>, boost::lockfree::capacity<100>> queue;
        std::thread thread;
    };

    const unsigned int num_threads;
    int next_worker = 0;
    std::vector<Worker> workers;
};

} // namespace ThreadPool

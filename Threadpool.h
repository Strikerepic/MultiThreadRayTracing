#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
    {
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        cv_.wait(lock, [this] {
                            return !tasks_.empty() || stop_;
                        });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                    // Mark task as completed
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        --active_tasks_;
                        totalTasksCompleted++;
                        if (tasks_.empty() && !stop_) {
                            cv_.notify_all(); // Notify when all tasks are completed
                        }
                    }
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();

        for (auto& thread : threads_) {
            thread.join();
        }
    }

    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::move(task));
            ++active_tasks_;
        }
        cv_.notify_one();
    }

    bool all_tasks_completed() const
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.empty() && active_tasks_ == 0;
    }

    int getTTC() const {
        return totalTasksCompleted;
    }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
    mutable int active_tasks_ = 0;
    int totalTasksCompleted = 0;
};

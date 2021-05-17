#pragma once

#include <queue>
#include <thread>
#include <vector>

#include <ctime>
#include <iomanip>
#include <sstream>

class Executor {
public:
    enum Status { kRunning, kShutdown, kTerminate, kCancelled };
    using Task = std::function<void()>;

    Executor() = default;
    virtual ~Executor() = default;

    template <typename F, typename... Args> void submit(F &&f, Args &&...args) {
        std::unique_lock<std::mutex> lck(mtx_);
        tasks_.push([f, args...]() { f(args...); });
        lck.unlock();
        tasks_cv_.notify_one();
        Schedule();
    }

    // No more tasks, but the current task in queue will be executed.
    virtual void shutdown() {
        std::lock_guard<std::mutex> lck(mtx_);
        if (status_ == kRunning) {
            status_ = kShutdown;
            tasks_cv_.notify_all();
        }
    }

    // The task which was currently running will be finished.
    virtual void terminate() {
        std::lock_guard<std::mutex> lck(mtx_);
        if (status_ < kTerminate) {
            status_ = kTerminate;
            tasks_cv_.notify_all();
        }
    }

    // Cancel the execution immedially, may downgrade to terminate.
    virtual void cancel() = 0;

    Status status() const {
        std::lock_guard<std::mutex> lck(mtx_);
        return status_;
    }

protected:
    virtual void Schedule() = 0;

    Status status_{kRunning};
    mutable std::mutex mtx_;
    std::condition_variable tasks_cv_;
    std::queue<Task> tasks_;
};

class SingleThreadExecutor : public Executor {
public:
    SingleThreadExecutor();

    ~SingleThreadExecutor();

    void cancel() override;

private:
    void Run();

    void Schedule() override {}

    bool waiting_{false};
    std::condition_variable no_waiting_cv_;
    std::thread th_;
};

class FixedThreadPoolExecutor : public Executor {
public:
    FixedThreadPoolExecutor(
            size_t numThreads = std::thread::hardware_concurrency());

    ~FixedThreadPoolExecutor();

    void cancel() override;

private:
    void Run();

    void Schedule() override {}

    size_t waiting_{0};
    std::condition_variable no_waiting_cv_;
    std::vector<std::thread> workers_;
};

class ScheduledThreadPoolExecutor : public Executor {
public:
    enum ThreadStatus {
        kThreadEmpty,
        kThreadStarting,
        kThreadRunning,
        kThreadExiting,
    };
    struct Thread {
        ThreadStatus status{kThreadEmpty};
        std::thread th;
    };

    ScheduledThreadPoolExecutor(
            size_t reserve_threads = std::thread::hardware_concurrency(),
            size_t max_threads = std::thread::hardware_concurrency() * 2,
            size_t initial_threads = 0,
            std::chrono::milliseconds idle_time =
                    std::chrono::milliseconds(1000));

    ~ScheduledThreadPoolExecutor();

    void cancel() override;

private:
    void Start(Thread *th);

    void Run(Thread *th);

    void Schedule() override;

    const size_t reserve_threads_;
    const size_t max_threads_;
    const std::chrono::milliseconds idle_time_;

    size_t running_{0};
    size_t waiting_{0};
    size_t exiting_{0};
    std::condition_variable no_waiting_cv_;
    std::vector<Thread *> workers_;
};

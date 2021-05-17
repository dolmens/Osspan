#include "executor.h"
namespace {} // namespace

SingleThreadExecutor::SingleThreadExecutor() : th_([this]() { Run(); }) {
}

SingleThreadExecutor::~SingleThreadExecutor() {
    shutdown();
    th_.join();
}

void SingleThreadExecutor::Run() {
    for (;;) {
        std::unique_lock<std::mutex> lck(mtx_);
        waiting_ = true;
        tasks_cv_.wait(lck, [this]() {
            return !tasks_.empty() || status_ != kRunning;
        });
        waiting_ = false;
        no_waiting_cv_.notify_one();
        if (status_ == kTerminate || status_ == kCancelled) {
            break;
        }
        if (!tasks_.empty()) {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            lck.unlock();
            task();
        } else if (status_ == kShutdown) {
            break;
        }
    }
}

void SingleThreadExecutor::cancel() {
    std::unique_lock<std::mutex> lck(mtx_);
    if (status_ != kCancelled) {
        status_ = kCancelled;
        tasks_cv_.notify_one();
        no_waiting_cv_.wait(lck, [this]() { return !waiting_; });
        pthread_cancel(th_.native_handle());
    }
}

FixedThreadPoolExecutor::FixedThreadPoolExecutor(size_t numThreads) {
    for (size_t i = 0; i < numThreads; i++) {
        workers_.emplace_back([this]() { Run(); });
    }
}

FixedThreadPoolExecutor::~FixedThreadPoolExecutor() {
    shutdown();
    for (auto &th : workers_) {
        th.join();
    }
}

void FixedThreadPoolExecutor::Run() {
    for (;;) {
        std::unique_lock<std::mutex> lck(mtx_);
        waiting_++;
        tasks_cv_.wait(lck, [this]() {
            return !tasks_.empty() || status_ != kRunning;
        });
        waiting_--;
        if (waiting_ == 0) {
            no_waiting_cv_.notify_one();
        }
        if (status_ == kTerminate || status_ == kCancelled) {
            break;
        }
        if (!tasks_.empty()) {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            lck.unlock();
            task();
        } else if (status_ == kShutdown) {
            break;
        }
    }
}

void FixedThreadPoolExecutor::cancel() {
    std::unique_lock<std::mutex> lck(mtx_);
    if (status_ != kCancelled) {
        status_ = kCancelled;
        tasks_cv_.notify_all();
        no_waiting_cv_.wait(lck, [this]() { return waiting_ == 0; });
        for (auto &th : workers_) {
            pthread_cancel(th.native_handle());
        }
    }
}

ScheduledThreadPoolExecutor::ScheduledThreadPoolExecutor(
        size_t reserve_threads,
        size_t max_threads,
        size_t initial_threads,
        std::chrono::milliseconds idle_time)
    : reserve_threads_(reserve_threads),
      max_threads_(max_threads),
      idle_time_(idle_time) {
    std::lock_guard<std::mutex> lck(mtx_);
    for (size_t i = 0; i < initial_threads; i++) {
        workers_.push_back(new Thread);
        Start(workers_[i]);
    }
}

ScheduledThreadPoolExecutor::~ScheduledThreadPoolExecutor() {
    shutdown();
    for (auto *th : workers_) {
        if (th->th.joinable()) {
            th->th.join();
        }
        delete th;
    }
}

void ScheduledThreadPoolExecutor::Start(Thread *th) {
    running_++;
    th->status = kThreadStarting;
    th->th = std::thread([this, th]() { Run(th); });
}

void ScheduledThreadPoolExecutor::Run(Thread *th) {
    std::unique_lock<std::mutex> lck(mtx_, std::defer_lock);
    th->status = kThreadRunning;

    for (;;) {
        lck.lock();
        waiting_++;
        tasks_cv_.wait_for(lck, idle_time_, [this]() {
            return !tasks_.empty() || status_ != kRunning;
        });
        waiting_--;
        if (waiting_ == 0) {
            no_waiting_cv_.notify_one();
        }
        if (status_ == kTerminate || status_ == kCancelled) {
            break;
        }
        if (!tasks_.empty()) {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            lck.unlock();
            task();
        } else if (status_ == kShutdown || running_ > reserve_threads_) {
            break;
        } else {
            // wait timeout, poll again
            lck.unlock();
        }
    }

    th->status = kThreadExiting;
    running_--;
    exiting_++;
}

void ScheduledThreadPoolExecutor::Schedule() {
    std::this_thread::yield();
    std::lock_guard<std::mutex> lck(mtx_);

    if (exiting_) {
        for (auto *th : workers_) {
            if (th->status == kThreadExiting) {
                th->th.join();
                th->status = kThreadEmpty;
                exiting_--;
                if (exiting_ == 0) {
                    break;
                }
            }
        }
    }

    if (!tasks_.empty() && running_ < max_threads_) {
        for (size_t i = 0; i < max_threads_; i++) {
            if (i == workers_.size()) {
                workers_.push_back(new Thread);
            }
            Thread *th = workers_[i];
            if (th->status == kThreadEmpty) {
                Start(th);
                return;
            }
        }
    }
}

void ScheduledThreadPoolExecutor::cancel() {
    std::unique_lock<std::mutex> lck(mtx_);
    if (status_ != kCancelled) {
        status_ = kCancelled;
        tasks_cv_.notify_all();
        no_waiting_cv_.wait(lck, [this]() { return waiting_ == 0; });
        for (auto *th : workers_) {
            if (th->status == kThreadRunning) {
                pthread_cancel(th->th.native_handle());
            }
        }
    }
}

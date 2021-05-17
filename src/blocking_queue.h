#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

template <typename T> class BlockingQueue {
public:
    enum Status { kOk, kEmpty, kShutdown, kTerminated };
    Status push(T &&v) {
        std::lock_guard<std::mutex> lck(mtx_);
        if (status_ >= kShutdown) {
            return status_;
        }
        q_.push(std::forward<T>(v));
        cv_.notify_one();
        return kOk;
    }

    Status push(const T &v) {
        std::lock_guard<std::mutex> lck(mtx_);
        if (status_ >= kShutdown) {
            return status_;
        }
        q_.push(v);
        cv_.notify_one();
        return kOk;
    }

    Status pop(T &value) {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this]() { return !q_.empty() || status_ >= kShutdown; });
        if (status_ == kTerminated) {
            return kTerminated;
        }
        if (q_.empty()) {
            return status_ == kShutdown ? kShutdown : kEmpty;
        }
        value = std::move(q_.front());
        q_.pop();
        return kOk;
    }

    Status try_pop(T &value) {
        std::unique_lock<std::mutex> lck(mtx_);
        if (status_ == kTerminated) {
            return kTerminated;
        }
        if (q_.empty()) {
            return status_ == kShutdown ? kShutdown : kEmpty;
        }
        value = std::move(q_.front());
        q_.pop();
        return kOk;
    }

    template <class Duration> Status try_pop(T &value, Duration timeout) {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait_for(lck, timeout, [this]() {
            return !q_.empty() || status_ >= kShutdown;
        });
        if (status_ == kTerminated) {
            return kTerminated;
        }
        if (q_.empty()) {
            return status_ == kShutdown ? kShutdown : kEmpty;
        }
        value = std::move(q_.front());
        q_.pop();
        return kOk;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lck(mtx_);
        status_ = kShutdown;
        cv_.notify_all();
    }

    void terminate() {
        std::lock_guard<std::mutex> lck(mtx_);
        status_ = kTerminated;
        cv_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lck(mtx_);
        return q_.size();
    }

    bool is_empty() const {
        std::lock_guard<std::mutex> lck(mtx_);
        return q_.empty();
    }

    Status status() {
        std::lock_guard<std::mutex> lck(mtx_);
        return status_;
    }

private:
    Status status_{kOk};
    std::queue<T> q_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
};

#pragma once

#include <atomic>

enum class Direction {
    Recv,
    Send,
};

class Traffic {
public:
    Traffic() : running_(false) {}

    Traffic(Direction direction) : running_(true), direction_(direction) {
        traffics[(int)direction]++;
    }

    ~Traffic() { Release(); }

    void Set(Direction direction) {
        if (!running_) {
            running_ = true;
            direction_ = direction;
            traffics[(int)direction]++;
        }
    }

    void Release() {
        if (running_) {
            running_ = false;
            if (--traffics[(int)direction_] < 0) {
                traffics[(int)direction_] = 0;
            }
        }
    }

    static bool IsOn(Direction direction) {
        return traffics[(int)direction] > 0;
    }

private:
    bool running_;
    Direction direction_;
    static std::atomic_int traffics[2];
};


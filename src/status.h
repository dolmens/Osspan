#pragma once

#include <string>

enum ErrorCode {
    EC_OK = 0,
    EC_FAIL = 1,
};

class Status {
public:
    static Status OK() { return Status(); }

    Status() {}

    Status(ErrorCode code, const std::string &message)
        : code_(code), message_(message) {}

    virtual ~Status() = default;

    bool ok() const { return code_ == EC_OK; }

private:
    ErrorCode code_{EC_OK};
    std::string message_;
};

#define RETURN_IF_FAIL(status) if (!status.ok()) { return status; }

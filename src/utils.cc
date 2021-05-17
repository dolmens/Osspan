#include <chrono>
#include <iomanip>
#include <sstream>

#include "utils.h"

std::string getTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               now.time_since_epoch()) %
                       1000;
    std::stringstream nowSs;
    nowSs << std::put_time(std::localtime(&nowAsTimeT), "%T") << '.'
          << std::setfill('0') << std::setw(3) << nowMs.count();
    return nowSs.str();
}

std::vector<std::string> split(const std::string &s,
                               const std::string &t,
                               size_t max) {
    std::vector<std::string> out;
    size_t i = 0;
    size_t start = 0;
    size_t end;

    for (;;) {
        if (++i == max) {
            out.push_back(s.substr(start));
            break;
        }
        end = s.find(t, start);
        out.push_back(s.substr(start, end - start));
        if (end != std::string::npos) {
            start = end + t.size();
        } else {
            break;
        }
    }

    return out;
}

std::string join(const std::vector<std::string> &vec,
                 const std::string &delim) {
    std::ostringstream ss;
    for (size_t i = 0; i < vec.size(); i++) {
        if (i > 0) {
            ss << ",";
        }
        ss << vec[i];
    }
    return ss.str();
}

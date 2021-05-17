#pragma once

#include <string>
#include <vector>

std::string getTimestamp();

std::vector<std::string> split(const std::string &s,
                               const std::string &t,
                               size_t max = 0);

std::string join(const std::vector<std::string> &vec, const std::string &delim);

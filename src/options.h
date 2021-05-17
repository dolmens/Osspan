#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

enum Option {
    OPTION_MAINWINDOW_POSITION,
    OPTION_SPLITTERTOP_SPLIT,
    OPTION_SPLITTERTOP_POSITION,
    OPTION_LASTOPEN_SITES,
    OPTION_LASTOPEN_INDEX,
    OPTION_DELETE_FINISHEDTASKS,
    OPTION_DOWNLOAD_THREAD_COUNT,
    OPTION_UPLOAD_THREAD_COUNT,
    OPTION_DOWNLOAD_SPEED_LIMIT,
    OPTION_DOWNLOAD_SPEED_ENABLE,
    OPTION_UPLOAD_SPEED_LIMIT,
    OPTION_UPLOAD_SPEED_ENABLE,
};

enum OptionType {
    OTNumber,
    OTString,
    OTNumberVec,
    OTStringVec,
};

struct OptionDef {
    std::string name;
    OptionType type_{};
    // validator
};

using OptionValue = std::
        variant<int, std::string, std::vector<int>, std::vector<std::string>>;

struct OptionRegistrator {
    OptionRegistrator(void (*f)()) { f(); }
};

class Options {
public:
    Options();
    ~Options();

    bool get_bool(Option option);
    int get_int(Option option);
    const std::string &get_string(Option option);
    const std::vector<int> &get_int_vec(Option option);
    const std::vector<std::string> &get_string_vec(Option option);

    void set(Option option, bool b);
    void set(Option option, int n);
    void set(Option option, const std::string &s);
    void set(Option option, const char *s);
    void set(Option option, const std::vector<int> &v);
    void set(Option option, const std::vector<std::string> &v);

    void Save();

private:
    void Load();

    std::vector<OptionDef> options_;
    std::vector<OptionValue> values_;
    std::map<std::string, size_t> indexes_;
};

Options &options();

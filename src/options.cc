#include "options.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "osspanapp.h"

namespace {
const std::string &empty_string() {
    static std::string empty;
    return empty;
}

const std::vector<int> &empty_int_vec() {
    static std::vector<int> v;
    return v;
}

const std::vector<std::string> &empty_string_vec() {
    static std::vector<std::string> v;
    return v;
}
} // namespace

Options::Options() {
    options_ = {
            {"OPTION_MAINWINDOW_POSITION", OTNumberVec},
            {"OPTION_SPLITTERTOP_SPLIT", OTNumber},
            {"OPTION_SPLITTERTOP_POSITION", OTNumber},
            {"OPTION_LASTOPEN_SITES", OTStringVec},
            {"OPTION_LASTOPEN_INDEX", OTNumber},
            {"OPTION_DELETE_FINISHEDTASKS", OTNumber},
            {"OPTION_DOWNLOAD_THREAD_COUNT", OTNumber},
            {"OPTION_UPLOAD_THREAD_COUNT", OTNumber},
            {"OPTION_DOWNLOAD_SPEED_LIMIT", OTNumber},
            {"OPTION_DOWNLOAD_SPEED_ENABLE", OTNumber},
            {"OPTION_UPLOAD_SPEED_LIMIT", OTNumber},
            {"OPTION_UPLOAD_SPEED_ENABLE", OTNumber},
    };

    values_ = {
            std::vector<int>{},
            0,
            0,
            std::vector<std::string>{},
            -1,
            0,
            4,
            4,
            0,
            0,
            0,
            0,
    };

    for (size_t i = 0; i < options_.size(); i++) {
        const auto &def = options_[i];
        indexes_[def.name] = i;
    }

    Load();
}

Options::~Options() {
}

bool Options::get_bool(Option option) {
    return get_int(option) != 0;
}

int Options::get_int(Option option) {
    size_t index = static_cast<size_t>(option);
    if (index < options_.size()) {
        const OptionDef &def = options_[index];
        if (def.type_ == OTNumber) {
            return std::get<int>(values_[index]);
        }
    }
    return 0;
}

const std::string &Options::get_string(Option option) {
    size_t index = static_cast<size_t>(option);
    if (index < values_.size()) {
        const OptionDef &def = options_[index];
        if (def.type_ == OTString) {
            return std::get<std::string>(values_[index]);
        }
    }
    return empty_string();
}

const std::vector<int> &Options::get_int_vec(Option option) {
    size_t index = static_cast<size_t>(option);
    if (index < options_.size()) {
        const OptionDef &def = options_[index];
        if (def.type_ == OTNumberVec) {
            return std::get<std::vector<int>>(values_[index]);
        }
    }
    return empty_int_vec();
}

const std::vector<std::string> &Options::get_string_vec(Option option) {
    size_t index = static_cast<size_t>(option);
    if (index < options_.size()) {
        const OptionDef &def = options_[index];
        if (def.type_ == OTStringVec) {
            return std::get<std::vector<std::string>>(values_[index]);
        }
    }
    return empty_string_vec();
}

void Options::set(Option option, bool b) {
    set(option, b ? 1 : 0);
}

void Options::set(Option option, int n) {
    size_t index = static_cast<size_t>(option);
    if (index < values_.size()) {
        OptionDef &def = options_[index];
        OptionValue &value = values_[index];
        if (def.type_ == OTNumber) {
            value = n;
        }
    }
}

void Options::set(Option option, const std::string &s) {
    size_t index = static_cast<size_t>(option);
    if (index < values_.size()) {
        OptionDef &def = options_[index];
        OptionValue &value = values_[index];
        if (def.type_ == OTString) {
            value = s;
        }
    }
}

void Options::set(Option option, const char *s) {
    set(option, std::string(s));
}

void Options::set(Option option, const std::vector<int> &v) {
    size_t index = static_cast<size_t>(option);
    if (index < values_.size()) {
        OptionDef &def = options_[index];
        OptionValue &value = values_[index];
        if (def.type_ == OTNumberVec) {
            value = v;
        }
    }
}

void Options::set(Option option, const std::vector<std::string> &v) {
    size_t index = static_cast<size_t>(option);
    if (index < values_.size()) {
        OptionDef &def = options_[index];
        OptionValue &value = values_[index];
        if (def.type_ == OTStringVec) {
            value = v;
        }
    }
}

void Options::Load() {
    std::string dataPath = wxGetApp().GetUserDataDir().ToStdString();
    std::string configFile = dataPath + "/Osspan.yml";
    if (!std::filesystem::exists(configFile)) {
        return;
    }
    YAML::Node root = YAML::LoadFile(configFile);
    for (YAML::const_iterator it = root.begin(); it != root.end(); it++) {
        auto key = it->first.as<std::string>();
        const auto indexit = indexes_.find(key);
        if (indexit == indexes_.end()) {
            continue;
        }
        size_t index = indexit->second;
        OptionDef &def = options_[index];
        OptionValue &value = values_[index];
        switch (def.type_) {
        case OTNumber:
            value = it->second.as<int>();
            break;
        case OTString:
            value = it->second.as<std::string>();
            break;
        case OTNumberVec:
            value = it->second.as<std::vector<int>>();
            break;
        case OTStringVec: {
            auto vs = it->second.as<std::vector<std::string>>();
            value = it->second.as<std::vector<std::string>>();
        } break;
        }
    }
}

void Options::Save() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    for (int i = 0; i < options_.size(); i++) {
        const OptionDef &def = options_[i];
        const OptionValue &value = values_[i];
        out << YAML::Key << def.name << YAML::Value;
        switch (def.type_) {
        case OTNumber:
            out << std::get<int>(value);
            break;
        case OTString:
            out << std::get<std::string>(value);
            break;
        case OTNumberVec:
            out << std::get<std::vector<int>>(value);
            break;
        case OTStringVec:
            out << std::get<std::vector<std::string>>(value);
            break;
        }
    }
    out << YAML::EndMap;
    std::string dataPath = wxGetApp().GetUserDataDir().ToStdString();
    std::string configFile = dataPath + "/Osspan.yml";
    std::ofstream fout(configFile);
    fout.write(out.c_str(), out.size());
    fout.close();
}

Options &options() {
    static Options options;
    return options;
}

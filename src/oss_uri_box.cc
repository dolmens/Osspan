#include "oss_uri_box.h"
#include "oss_client.h"

#include <filesystem>
#include <string_view>

std::string OssUriBox::Format() {
    std::string_view proto(OSSPROTOP, OSSPROTOPLEN);
    std::string path = GetValue().ToStdString();
    if (path.size() >= OSSPROTOPLEN &&
        std::equal(path.begin(),
                   path.begin() + OSSPROTOPLEN,
                   proto.begin(),
                   [](auto a, auto b) { return std::tolower(a) == b; })) {
        path = path.substr(OSSPROTOPLEN - 1);
    }
    if (path.front() != '/') {
        std::string currentPath = site_->GetCurrentPath();
        if (currentPath.empty()) {
            return currentPath;
        }
        path = currentPath.substr(OSSPROTOPLEN - 1) + path;
    }
    path = std::filesystem::weakly_canonical(path);
    if (path.empty()) {
        return path;
    }
    if (path.back() != '/') {
        path += '/';
    }
    path = std::string(OSSPROTOP, OSSPROTOPLEN) + path.substr(1);
    SetValue(path);
    return path;
}

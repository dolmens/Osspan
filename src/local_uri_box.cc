#include "local_uri_box.h"

#include <filesystem>
std::string LocalUriBox::Format() {
    std::string path = GetValue().ToStdString();
    if (path.front() != '/') {
        std::string currentPath = site_->GetCurrentPath();
        if (currentPath.empty()) {
            return currentPath;
        }
        path = currentPath + path;
    }
    path = std::filesystem::weakly_canonical(path);
    if (path.empty()) {
        return path;
    }
    if (path.back() != '/') {
        path += '/';
    }
    SetValue(path);
    return path;
}

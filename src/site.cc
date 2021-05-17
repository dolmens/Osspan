#include "site.h"
#include "utils.h"

#include <alibabacloud/oss/OssClient.h>

#include "global_executor.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <fstream>
#include <map>

#define HISTORYCAP 10

std::wstring fileTypeToStdWstring(FileType fileType) {
    switch (fileType) {
    case FTDirectory:
        return L"目录";
    case FTFile:
        return L"文件";
    }
    return L"";
}

DirPtr Dir::CopyBasic() {
    DirPtr dir(new Dir(path));

    dir->path = path;
    dir->tag = tag;

    dir->files.reserve(files.size());
    for (const auto &file : files) {
        FilePtr f(new File);
        *f = *file;
        dir->files.push_back(f);
    }

    return dir;
}

void DirectoryCenter::Register(DirectoryListener *listener) {
    std::lock_guard<std::mutex> lck(mtx_);
    if (std::find(listeners_.begin(), listeners_.end(), listener) ==
        listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void DirectoryCenter::Unregister(DirectoryListener *listener) {
    std::lock_guard<std::mutex> lck(mtx_);
    listeners_.erase(
            std::remove(listeners_.begin(), listeners_.end(), listener),
            listeners_.end());
}

void DirectoryCenter::NotifyDirectoryUpdated(const std::string &site,
                                             const std::string &path) {
    std::unique_lock<std::mutex> lck(mtx_);
    std::vector<DirectoryListener *> listeners = listeners_;
    lck.unlock();
    for (auto *listener : listeners) {
        listener->directoryUpdated(site, path);
    }
}

DirectoryCenter::DirectoryCenter() {
}

DirectoryCenter::~DirectoryCenter() {
}

DirectoryCenter *directoryCenter() {
    static DirectoryCenter inst;
    return &inst;
}

Site::Site(SiteType type) : type_(type), currentDir_(new Dir("")) {
}

Site::~Site() {
    if (directoryCenterWatched_) {
        directoryCenter()->Unregister(this);
    }
}

void Site::WatchDirectoryCenter() {
    directoryCenter()->Register(this);
    directoryCenterWatched_ = true;
}

void Site::Attach(SiteListener *l) {
    if (std::find(listeners_.begin(), listeners_.end(), l) ==
        listeners_.end()) {
        listeners_.push_back(l);
    }
}

void Site::Detach(SiteListener *l) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), l),
                     listeners_.end());
}

void Site::ChangeDir(const std::string &requestPath, SiteUpdatedCallback cb) {
    std::string path = requestPath;
    if (path.back() != '/') {
        path += '/';
    }

    if (currentDir_->path.empty()) {
        currentDir_->path = path;
    }

    if (is_updating()) {
        if (!is_refreshing()) {
            wxBell();
            return;
        }
    }

    updatingPath_ = path;
    pendingPath_ = "";
    globalExecutor()->submit([this, path, cb]() {
        DirPtr dir;
        Status status = GetDir(path, dir);
        wxTheApp->CallAfter([this, status, path, dir, cb]() {
            if (updatingPath_ == path) {
                updatingPath_ = "";
                if (status.ok()) {
                    if (currentDir_->path != dir->path) {
                        forwards_.clear();
                        backwards_.emplace_front(std::move(currentDir_));
                        if (backwards_.size() > HISTORYCAP) {
                            backwards_.pop_back();
                        }
                    }
                    currentDir_ = dir;
                    NotifyChanged();
                }

                if (cb) {
                    cb(this, status);
                }

                if (!pendingPath_.empty()) {
                    std::string pendingPath = std::move(pendingPath_);
                    // Currently can only pending refresh without callback
                    assert(pendingPath == dir->path);
                    ChangeDir(pendingPath);
                }
            }
        });
    });
}

void Site::ChangeToSubDir(const std::string &subdir, SiteUpdatedCallback cb) {
    ChangeDir(GetCurrentPath() + subdir + "/", cb);
}

void Site::Up(SiteUpdatedCallback cb) {
    if (!IsRoot()) {
        std::string parentPath = GetParentPath();
        ChangeDir(parentPath, cb);
    }
}

void Site::Refresh(bool immediately, SiteUpdatedCallback cb) {
    if (is_updating()) {
        if (is_refreshing()) {
            if (immediately) {
                pendingPath_ = "";
            } else {
                pendingPath_ = GetCurrentPath();
                return;
            }
        } else {
            return;
        }
    }

    ChangeDir(GetCurrentPath(), cb);
}

void Site::Backward() {
    if (is_updating() && !is_refreshing()) {
        wxBell();
        return;
    }

    updatingPath_ = "";
    pendingPath_ = "";

    if (!backwards_.empty()) {
        forwards_.push_front(std::move(currentDir_));
        currentDir_ = std::move(backwards_.front());
        backwards_.pop_front();
        NotifyChanged();
    }
}

void Site::Forward() {
    if (is_updating() && !is_refreshing()) {
        wxBell();
        return;
    }

    updatingPath_ = "";
    pendingPath_ = "";

    if (!forwards_.empty()) {
        backwards_.push_front(std::move(currentDir_));
        currentDir_ = std::move(forwards_.front());
        forwards_.pop_front();
        NotifyChanged();
    }
}

void Site::directoryUpdated(const std::string &site, const std::string &path) {
    if (name() == site && currentDir_->path == path) {
        Refresh();
    }
}

std::string Site::GetBackwardPath() const {
    if (!backwards_.empty()) {
        return backwards_.front()->path;
    }
    return "";
}

std::string Site::GetForwardPath() const {
    if (!forwards_.empty()) {
        return forwards_.front()->path;
    }
    return "";
}

std::pair<std::string, std::string> Site::Split(const std::string &path) {
    size_t pos = path.find_last_of('/');
    assert(pos != path.npos);
    if (pos != path.size() - 1 || pos == 0) {
        return {path.substr(0, pos + 1), path.substr(pos + 1)};
    } else {
        size_t start = path.find_last_of('/', pos - 1);
        assert(start != path.npos);
        return {path.substr(0, start + 1), path.substr(start + 1, pos - start)};
    }
}

std::string Site::NamePart(const std::string &path) {
    size_t pos = path.find_last_of('/');
    assert(pos != path.npos);
    if (pos != path.size() - 1 || pos == 0) {
        return path.substr(pos + 1);
    } else {
        size_t start = path.find_last_of('/', pos - 1);
        assert(start != path.npos);
        return path.substr(start + 1, pos - start - 1);
    }
}

std::string Site::Combine(const std::string &base, const FilePtr &file) {
    return base + file->name + (file->type == FTDirectory ? "/" : "");
}

std::string Site::Combine(const std::string &base, const std::string &path) {
    return base + path;
}

void Site::NotifyChanged() {
    for (auto *l : listeners_) {
        l->SiteUpdated(this);
    }
}

bool IsSubDir(const std::string &base, const std::string &path) {
    return base.size() <= path.size() &&
           !std::strncmp(base.c_str(), path.c_str(), base.size());
}

std::string RelativePath(const std::string &base, const std::string &path) {
    return path.substr(base.size());
}

#include "dndobjects.h"
#include "file_list_ctrl.h"
#include "local_list_ctrl.h"
#include "oss_list_ctrl.h"

#include <filesystem>

FileDataObject::FileDataObject()
    : wxDataObjectSimple(wxDataFormat("OsspanFileDataObject")) {
}

FileDataObject::~FileDataObject() {
}

size_t FileDataObject::GetDataSize() const {
    size_t ret = 1;
    ret += site_.size() + 1;
    for (const auto &file : files_) {
        ret += file.first.size() + 1 + sizeof(file.second.size) +
               sizeof(file.second.lastModifiedTime) + file.second.etag.size() +
               1;
    }

    return ret;
}

bool FileDataObject::GetDataHere(void *buf) const {
    auto p = static_cast<char *>(buf);
    *p = 1;
    ++p;
    strcpy(p, site_.c_str());
    p += site_.size() + 1;
    for (const auto &file : files_) {
        strcpy(p, file.first.c_str());
        p += file.first.size() + 1;
        std::memcpy(p, &file.second.size, sizeof(file.second.size));
        p += sizeof(file.second.size);
        std::memcpy(p,
                    &file.second.lastModifiedTime,
                    sizeof(file.second.lastModifiedTime));
        p += sizeof(file.second.lastModifiedTime);
        std::strcpy(p, file.second.etag.c_str());
        p += file.second.etag.size() + 1;
    }

    return true;
}

bool FileDataObject::SetData(size_t len, const void *buf) {
    files_.clear();

    auto p = static_cast<const char *>(buf);
    const auto end = p + len;
    if (!len || !buf) {
        return false;
    }

    if (p[0] != 1) {
        return false;
    }
    ++p;

    size_t slen = strlen(p);
    if (slen) {
        site_ = std::string(p, p + slen);
    }
    p += slen + 1;

    while (p < end) {
        size_t slen = strlen(p);
        std::string s(p, p + slen);
        p += slen + 1;
        FileStat st;
        std::memcpy(&st.size, p, sizeof(st.size));
        p += sizeof(st.size);
        std::memcpy(&st.lastModifiedTime, p, sizeof(st.lastModifiedTime));
        p += sizeof(st.lastModifiedTime);
        std::string etag(p);
        p += etag.size() + 1;
        st.etag = std::move(etag);
        files_.emplace_back(s, st);
    }

    return true;
}

template <class FileListCtrl>
wxDragResult FileDropTarget<FileListCtrl>::OnData(wxCoord x,
                                                  wxCoord y,
                                                  wxDragResult def) {
    def = FixupDragResult(def);
    int flags;
    long hit = fileListCtrl_->HitTest(wxPoint(x, y), flags);
    std::string dstDirectory = fileListCtrl_->GetCurrentPath();
    if (hit != wxNOT_FOUND) {
        const FilePtr &file = fileListCtrl_->GetData(hit);
        if (file->type == FTDirectory) {
            dstDirectory += file->name + "/";
        }
    }

    if (!GetData()) {
        return wxDragError;
    }

    std::string srcSite;
    std::vector<std::pair<std::string, FileStat>> files;
    auto format = dataObject_->GetReceivedFormat();
    if (format == fileDataObject_->GetFormat()) {
        srcSite = fileDataObject_->GetSite();
        files = fileDataObject_->GetFiles();
    } else {
        const auto &sysFiles = sysFileDataObject_->GetFilenames();
        for (const auto &wxf : sysFiles) {
            std::string f = wxf.ToStdString();
            namespace fs = std::filesystem;
            std::error_code ec;
            auto status = fs::status(f, ec);
            if (ec) {
                continue;
            }
            size_t size;
            std::time_t cftime;
            if (status.type() == fs::file_type::directory) {
                f += '/';
                size = 0;
                cftime = 0;
            } else {
                size = fs::file_size(f, ec);
                if (ec) {
                    continue;
                }
                auto ftime = fs::last_write_time(f, ec);
                if (ec) {
                    continue;
                }
                cftime = decltype(ftime)::clock::to_time_t(ftime);
            }
            files.emplace_back(std::move(f), FileStat{size, cftime});
        }
    }

    TaskPtrVec tasks;
    const std::string &dstSite = fileListCtrl_->GetSite()->name();
    const TaskPtr &ossTask =
            taskList()->GetOssTask(srcSite.empty() ? dstSite : srcSite);
    for (const auto &f : files) {
        const std::string &srcPath = f.first;
        const FileStat &fileStat = f.second;

        const auto [srcDirectory, _] = Site::Split(f.first);
        if (srcDirectory == dstDirectory ||
            (srcDirectory.size() < dstDirectory.size() &&
             !strncmp(srcDirectory.c_str(),
                      dstDirectory.c_str(),
                      srcDirectory.size()))) {
            continue;
        }

        std::string dstPath = dstDirectory + Site::NamePart(srcPath);
        if (srcPath.back() == '/') {
            dstPath += '/';
        }

        TaskPtr task(new Task);
        task->type = TTCopy;
        task->status = TSPending;
        task->srcSite = srcSite;
        task->srcPath = srcPath;
        task->dstSite = dstSite;
        task->dstPath = dstPath;
        task->fileStat = fileStat;
        task->parentId = ossTask->id;
        task->parent = ossTask;
        tasks.push_back(task);
    }

    taskList()->AddTask(ossTask, tasks);

    return def;
}

template class FileDropTarget<LocalListCtrl>;
template class FileDropTarget<OssListCtrl>;

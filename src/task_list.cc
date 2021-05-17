#include "task_list.h"
#include "directory_compare.h"
#include "global_executor.h"
#include "options.h"
#include "oss_site.h"
#include "osspanapp.h"
#include "schedule_list.h"
#include "storage.h"
#include "traffic.h"
#include "utils.h"

#include <fstream>

TaskList *taskList() {
    static std::shared_ptr<TaskList> taskList(new TaskList);
    return taskList.get();
}

TaskList::TaskList() {
    int downloadThreadCount = options().get_int(OPTION_DOWNLOAD_THREAD_COUNT);
    tpDownload_.reset(new ScheduledThreadPoolExecutor(
            downloadThreadCount / 2, downloadThreadCount, 1));
    int uploadThreadCount = options().get_int(OPTION_UPLOAD_THREAD_COUNT);
    tpUpload_.reset(new ScheduledThreadPoolExecutor(
            uploadThreadCount / 2, uploadThreadCount, 1));
    root_.reset(new Task);
    root_->type = TTRoot;
    root_->status = TSNone;
}

TaskList::~TaskList() {
}

namespace {
SitePtr createSite(const std::string &name) {
    if (name.empty()) {
        return std::make_shared<LocalSite>();
    }
    return std::make_shared<OssSite>(name);
}
} // namespace

void TaskList::Attach(TaskListListener *listener) {
    Detach(listener);
    listeners_.push_back(listener);
}

void TaskList::Detach(TaskListListener *listener) {
    listeners_.erase(
            std::remove(listeners_.begin(), listeners_.end(), listener),
            listeners_.end());
}

void TaskList::AddTask(TaskType type,
                       const std::string &srcSite,
                       const std::string &srcPath,
                       const std::string &dstSite,
                       const std::string &dstPath,
                       const FileStat &stat,
                       int flag) {
    AddTask(type, 0, srcSite, srcPath, dstSite, dstPath, stat, flag);
}

void TaskList::AddTask(TaskType type,
                       long scheduleId,
                       const std::string &srcSite,
                       const std::string &srcPath,
                       const std::string &dstSite,
                       const std::string &dstPath,
                       const FileStat &stat,
                       int flag) {
    const std::string &siteName = !srcSite.empty() ? srcSite : dstSite;
    const TaskPtr &parent = GetOssTask(siteName);

    TaskPtr task(new Task);
    task->scheduleId = scheduleId;
    task->type = type;
    task->status = TSPending;
    task->srcSite = srcSite;
    task->srcPath = srcPath;
    task->dstSite = dstSite;
    task->dstPath = dstPath;
    task->fileStat = stat;
    task->flag = flag;

    AddTask(parent, task);
}

void TaskList::AddTask(const TaskPtr &parent, const TaskPtr &task) {
    task->parentId = parent->id;
    task->parent = parent;
    parent->children.push_back(task);
    TaskAdded(parent, task);
    Submit(task);
}

void TaskList::AddTask(const TaskPtr &parent, const TaskPtrVec &tasks) {
    // children's append should do in main thread
    parent->children.insert(parent->children.end(), tasks.begin(), tasks.end());
    size_t total = 0;
    size_t dirCnt = 0;
    for (const auto &task : tasks) {
        assert(task->parent.lock() == parent);
        if (task->srcPath.back() == '/') {
            dirCnt++;
        } else {
            total += task->fileStat.size;
        }
    }
    // 先设定所有目录的size之和等于所有文件的size之和
    size_t oneDirSize = total * 1.0 / dirCnt;
    for (auto &task : tasks) {
        if (task->srcPath.back() == '/') {
            task->fileStat.size = oneDirSize;
        }
    }
    total += oneDirSize * dirCnt;
    int64_t diff = total - parent->fileStat.size;
    for (TaskPtr ancestor = parent; ancestor->type != TTSite;
         ancestor = ancestor->parent.lock()) {
        ancestor->fileStat.size += diff;
        TaskUpdated(ancestor);
    }
    Submit(tasks);
    TaskAdded(parent, tasks);
}

const TaskPtr &TaskList::GetOssTask(const std::string &ossSiteName) {
    auto it = std::find_if(root_->children.begin(),
                           root_->children.end(),
                           [&ossSiteName](const auto &p) {
                               return p->srcSite == ossSiteName;
                           });
    if (it == root_->children.end()) {
        TaskPtr site(new Task);
        site->type = TTSite;
        site->status = TSNone;
        site->srcSite = ossSiteName;
        site->parent = root_;
        root_->children.push_back(site);

        TaskAdded(root_, site);
        it = std::prev(root_->children.end());
    }

    return *it;
}

void TaskList::StopTask(const TaskPtr &task) {
    if (task->status < TSFinished) {
        if (task->children.empty()) {
            if (task->type != TTRoot && task->type != TTSite) {
                task->stop = true;
            }
        } else {
            for (const auto &child : task->children) {
                StopTask(child);
            }
        }
    }
}

void TaskList::ResumeTask(const TaskPtr &task) {
    if (task->status == TSStopped) {
        task->status = TSPending;
        // Resume only submit again when no children
        if (task->children.empty()) {
            Submit(task);
        } else {
            task->status = TSStarted;
            TaskUpdated(task);
            for (const auto &t : task->children) {
                ResumeTask(t);
            }
        }
    }
}

void TaskList::RestartTask(const TaskPtr &task) {
    if (task->status >= TSFinished && task->type != TTCopyPart) {
        if (task->status == TSStopped && task->type == TTCopy &&
            !task->uploadId.empty()) {
            SubmitCopyAbort(task);
        }
        for (const auto &t : task->children) {
            storage()->RemoveTask(t);
        }
        task->children.clear();
        task->status = TSPending;
        size_t progress = task->progress;
        for (Task *parent = task.get(); parent && parent->type != TTSite;
             parent = parent->parent.lock().get()) {
            parent->progress -= progress;
        }
        task->uploadId = "";
        TaskUpdated(task);
        Submit(task);
    }
}

void TaskList::RemoveTask(const TaskPtr &task) {
    assert(task->type != TTRoot);
    if (task->type == TTSite) {
        TaskPtrVec children = task->children;
        for (auto it = children.begin(); it != children.end(); it++) {
            if ((*it)->status >= TSFinished) {
                RemoveTask(*it);
            }
        }
        if (task->children.empty()) {
            auto taskIt = std::find(
                    root_->children.begin(), root_->children.end(), task);
            if (taskIt != root_->children.end()) {
                root_->children.erase(taskIt);
                TaskRemoved(task);
            }
        }
        return;
    }
    if (task->status >= TSFinished && task->type != TTCopyPart) {
        if (task->status == TSStopped && task->type == TTCopy &&
            !task->uploadId.empty()) {
            SubmitCopyAbort(task);
        }
        auto parent = task->parent.lock();

        auto taskIt = std::find(
                parent->children.begin(), parent->children.end(), task);
        if (taskIt != parent->children.end()) {
            parent->children.erase(taskIt);
            TaskRemoved(task);
        }
    }
}

void TaskList::Load() {
    root_->children = storage()->LoadTasks();
    for (const auto &site : root_->children) {
        site->parent = root_;
    }
    bool removeFinishedTasks = options().get_bool(OPTION_DELETE_FINISHEDTASKS);
    if (removeFinishedTasks) {
        RemoveFinishedTasks();
    }
}

void TaskList::Close() {
    for (const auto &task : root_->children) {
    }
    tpDownload_->cancel();
    tpDownload_.reset();
    tpUpload_->cancel();
    tpUpload_.reset();
}

void TaskList::Submit(const TaskPtr &task) {
    // 只有明确的上传使用上传线程池
    Executor *tp = tpDownload_.get();
    if (task->type == TTCopy &&
        (task->srcSite.empty() && !task->dstSite.empty())) {
        tp = tpUpload_.get();
    }
    tp->submit([this, task]() { Execute(task); });
}

void TaskList::Submit(const TaskPtrVec &tasks) {
    /*
     * If there are too many tasks, submit one by one may be slowly, so just
     * submit the total pack, then submit the items.
     */
    globalExecutor()->submit([this, tasks]() {
        for (const auto &t : tasks) {
            Submit(t);
        }
    });
}

void TaskList::SubmitCopyFinish(const TaskPtr &task) {
    tpDownload_->submit([this, task]() { ExecuteCopyFinish(task); });
}

void TaskList::SubmitCopyAbort(const TaskPtr &task) {
    tpDownload_->submit([this, task]() { ExecuteCopyAbort(task); });
}

void TaskList::Execute(const TaskPtr &task) {
    if (task->stop) {
        wxTheApp->CallAfter([this, task]() { TaskStopped(task); });
        return;
    }
    wxTheApp->CallAfter([this, task]() {
        TaskStarted(task);
        wxGetApp().mainFrame()->statusBar()->UpdateActivityLed();
    });

    SitePtr srcSite = createSite(task->srcSite);
    SitePtr dstSite = createSite(task->dstSite);
    if (!srcSite->IsOk() || !dstSite->IsOk()) {
        wxTheApp->CallAfter(
                [this, task]() { TaskFailed(task, Status(EC_FAIL, "")); });
        return;
    }

    // 测试代码
    // {
    //     using namespace std::chrono_literals;
    //     std::this_thread::sleep_for(10s);
    // }

    switch (task->type) {
    case TTSync:
        ExecuteSync(task, srcSite, dstSite);
        break;
    case TTCopy:
        ExecuteCopy(task, srcSite, dstSite);
        break;
    case TTCopyPart:
        ExecuteCopyPart(task, srcSite, dstSite);
        break;
    default:
        assert(0);
        break;
    }
}

void TaskList::ExecuteSync(const TaskPtr &task,
                           const SitePtr &srcSite,
                           const SitePtr &dstSite) {
    TaskPtrVec subTasks;
    Status status;
    DirPtr srcDir;
    DirPtr dstDir;
    status = srcSite->GetDir(task->srcPath, srcDir);
    Traffic traffic(Direction::Recv);
    status = dstSite->GetDir(task->dstPath, dstDir);
    if (CompareDirectory(srcSite,
                         srcDir,
                         dstSite,
                         dstDir,
                         !(task->flag & SYNC_ONLY_NEW)) != 0) {
        // The compareDirectory may GetETag, so release traffic here.
        traffic.Release();
        for (const auto &file : srcDir->files) {
            if (file->type == FTDirectory && file->name != ".." &&
                file->cmp == CSNone) {
                // 如果是目录且没有明确相等或者不等，则继续比较
                TaskPtr t(new Task);
                t->type = TTSync;
                t->status = TSPending;
                t->srcSite = task->srcSite;
                t->srcPath = Site::Combine(srcDir->path, file);
                t->dstSite = task->dstSite;
                t->dstPath = Site::Combine(dstDir->path, file);
                t->fileStat = {0, 0};
                t->flag = task->flag;
                t->parentId = task->id;
                t->parent = task;
                subTasks.push_back(t);
            }
        }

        if (task->flag & SYNC_UP) {
            for (const auto &file : srcDir->files) {
                if (file->cmp == CSNew || file->cmp == CSUpdated) {
                    TaskPtr t(new Task);
                    t->type = TTCopy;
                    t->status = TSPending;
                    t->srcSite = task->srcSite;
                    t->srcPath = Site::Combine(srcDir->path, file);
                    t->dstSite = task->dstSite;
                    t->dstPath = Site::Combine(dstDir->path, file);
                    t->fileStat = file->stat;
                    t->parentId = task->id;
                    t->parent = task;
                    subTasks.push_back(t);
                }
            }
        }

        if (task->flag & SYNC_DOWN) {
            for (const auto &file : dstDir->files) {
                if (file->cmp == CSNew || file->cmp == CSUpdated) {
                    TaskPtr t(new Task);
                    t->type = TTCopy;
                    t->status = TSPending;
                    t->srcSite = task->dstSite;
                    t->srcPath = Site::Combine(dstDir->path, file);
                    t->dstSite = task->srcSite;
                    t->dstPath = Site::Combine(srcDir->path, file);
                    t->fileStat = file->stat;
                    t->parentId = task->id;
                    t->parent = task;
                    subTasks.push_back(t);
                }
            }
        }
    }
    if (!subTasks.empty()) {
        wxTheApp->CallAfter(
                [this, task, subTasks]() { AddTask(task, subTasks); });
    } else {
        wxTheApp->CallAfter([this, task]() { TaskFinished(task, 0, 0); });
    }
}

#define SEGMENT (10 * 1000 * 1000)
#define UPLOAD_THREADS 1
#define DOWNLOAD_THREADS 1

void TaskList::ExecuteCopy(const TaskPtr &task,
                           const SitePtr &srcSite,
                           const SitePtr &dstSite) {
    if (task->srcPath.back() == '/') {
        ExecuteCopyDir(task, srcSite, dstSite);
    } else {
        if (task->fileStat.size >= SEGMENT) {
            ExecuteCopyBig(task, srcSite, dstSite);
        } else {
            Site *site =
                    srcSite->type() == STLocal ? dstSite.get() : srcSite.get();
            Traffic traffic(srcSite->type() == STLocal ? Direction::Send
                                                       : Direction::Recv);
            Status status =
                    site->Copy(task->srcPath, task->dstPath, task->fileStat);
            traffic.Release();
            if (status.ok()) {
                wxTheApp->CallAfter([this, task]() {
                    TaskFinished(task, task->fileStat.size);
                });
            } else {
                wxTheApp->CallAfter(
                        [this, task, status]() { TaskFailed(task, status); });
            }
        }
    }
}

void TaskList::ExecuteCopyDir(const TaskPtr &task,
                              const SitePtr &srcSite,
                              const SitePtr &dstSite) {
    Status status = dstSite->MakeDir(task->dstPath);
    if (!status.ok()) {
        wxTheApp->CallAfter(
                [this, task, status]() { TaskFailed(task, status); });
        return;
    }
    wxTheApp->CallAfter([this, task]() {
        auto [dstDirectory, _] = Site::Split(task->dstPath);
        directoryCenter()->NotifyDirectoryUpdated(task->dstSite, dstDirectory);
    });

    DirPtr srcDir;
    status = srcSite->GetDir(task->srcPath, srcDir);
    if (!status.ok()) {
        wxTheApp->CallAfter(
                [this, task, status]() { TaskFailed(task, status); });
        return;
    }
    TaskPtrVec subTasks;
    if (!srcDir->files.empty()) {
        for (const auto &file : srcDir->files) {
            if (file->name == "..") {
                continue;
            }
            TaskPtr t(new Task);
            t->type = TTCopy;
            t->status = TSPending;
            t->srcSite = task->srcSite;
            t->srcPath = Site::Combine(srcDir->path, file);
            t->dstSite = task->dstSite;
            t->dstPath = Site::Combine(task->dstPath, file);
            t->fileStat = file->stat;
            t->parentId = task->id;
            t->parent = task;
            subTasks.push_back(t);
        }
        wxTheApp->CallAfter(
                [this, task, subTasks]() { AddTask(task, subTasks); });
    } else {
        wxTheApp->CallAfter([this, task]() { TaskFinished(task, 0, 0); });
    }
}

void TaskList::ExecuteCopyBig(const TaskPtr &task,
                              const SitePtr &srcSite,
                              const SitePtr &dstSite) {
    if (srcSite->type() == STLocal && dstSite->type() == STOss) {
        OssSite *ossSite = (OssSite *)dstSite.get();
        std::string uploadId = task->uploadId;
        if (uploadId.empty()) {
            Traffic traffic(Direction::Send);
            Status status = ossSite->InitMultipartUpload(
                    task->srcPath, task->dstPath, uploadId);
            if (status.ok()) {
                wxTheApp->CallAfter([this, task, uploadId]() {
                    TaskProgressInit(task, uploadId, 0);
                });
            } else {
                wxTheApp->CallAfter(
                        [this, task, status]() { TaskFailed(task, status); });
                return;
            }
        }
        int nthreads = UPLOAD_THREADS;
        if (nthreads == 1) {
            // The uploadId have not been written to task, so pass it.
            ExecuteCopyFromLocalSegments(task, uploadId, srcSite, dstSite);
            return;
        } else {
            size_t perThreadSize = task->fileStat.size / nthreads;
            size_t perThreadParts = perThreadSize / SEGMENT;
            TaskPtrVec parts;
            for (int i = 0; i < nthreads; i++) {
                size_t size = i < nthreads - 1
                                      ? perThreadSize
                                      : perThreadSize +
                                                task->fileStat.size % nthreads;
                TaskPtr t(new Task);
                t->type = TTCopyPart;
                t->status = TSPending;
                t->srcSite = task->srcSite;
                t->srcPath = task->srcPath;
                t->dstSite = task->dstSite;
                t->dstPath = task->dstPath;
                t->fileStat = {size, 0};
                t->uploadId = uploadId;
                t->offset = perThreadSize * i;
                t->partId = perThreadParts * i;
                t->parentId = task->id;
                t->parent = task;
                parts.push_back(t);
            }
            wxTheApp->CallAfter(
                    [this, task, parts]() { AddTask(task, parts); });
        }
        return;
    } else if (srcSite->type() == STOss && dstSite->type() == STLocal) {
        int nthreads = DOWNLOAD_THREADS;
        if (nthreads == 1) {
            ExecuteCopyToLocalSegments(task, srcSite, dstSite);
        } else {
            size_t perThreadSize = task->fileStat.size / nthreads;
            std::string dstPath =
                    std::filesystem::path(task->dstPath)
                            .parent_path()
                            .string() +
                    std::string("/.") +
                    std::filesystem::path(task->dstPath).filename().string() +
                    ".osspanpart.";
            TaskPtrVec parts;
            for (int i = 0; i < nthreads; i++) {
                size_t size = perThreadSize;
                if (i == nthreads - 1) {
                    size += task->fileStat.size % nthreads;
                }
                TaskPtr t(new Task);
                t->type = TTCopyPart;
                t->status = TSPending;
                t->srcSite = task->srcSite;
                t->srcPath = task->srcPath;
                t->dstSite = task->dstSite;
                t->dstPath = dstPath + std::to_string(i + 1);
                t->fileStat = {size, 0};
                t->offset = perThreadSize * i;
                t->parentId = task->id;
                t->parent = task;
                parts.push_back(t);
            }
            wxTheApp->CallAfter(
                    [this, task, parts]() { AddTask(task, parts); });
        }
        return;
    }
    assert(0); // TODO.
}

void TaskList::ExecuteCopyFromLocalSegments(const TaskPtr &task,
                                            const std::string &uploadId,
                                            const SitePtr &srcSite,
                                            const SitePtr &dstSite) {
    OssSite *ossSite = (OssSite *)dstSite.get();
    size_t nparts = task->fileStat.size / SEGMENT;
    size_t partId = task->progress / SEGMENT;
    for (; partId < nparts; partId++) {
        // Give a chance to leave.
        if (task->stop) {
            wxTheApp->CallAfter([this, task]() { TaskStopped(task); });
            return;
        }
        size_t offset = partId * SEGMENT;
        size_t size =
                partId < nparts - 1 ? SEGMENT : (task->fileStat.size - offset);
        Traffic traffic(Direction::Send);
        Status status =
                ossSite->CopyFileFromLocalPart(task->srcPath,
                                               task->dstPath,
                                               uploadId,
                                               task->partId + partId + 1,
                                               task->offset + offset,
                                               size);
        if (status.ok()) {
            wxTheApp->CallAfter(
                    [this, task, size]() { TaskProgressUpdated(task, size); });
        } else {
            wxTheApp->CallAfter(
                    [this, task, status]() { TaskFailed(task, status); });
            return;
        }
    }

    Status status = Status::OK();
    if (task->type == TTCopy) {
        Traffic traffic(Direction::Send);
        status = ossSite->CopyFileFromLocalPartFinish(task->dstPath, uploadId);
    }
    if (status.ok()) {
        wxTheApp->CallAfter([this, task]() { TaskFinished(task, 0); });
    } else {
        wxTheApp->CallAfter(
                [this, task, status]() { TaskFailed(task, status); });
    }
}

void TaskList::ExecuteCopyToLocalSegments(const TaskPtr &task,
                                          const SitePtr &srcSite,
                                          const SitePtr &dstSite) {
    size_t progress = task->progress;
    if (std::filesystem::exists(task->dstPath)) {
        size_t fsize = std::filesystem::file_size(task->dstPath);
        if (fsize > progress) {
            std::filesystem::resize_file(task->dstPath, progress);
        } else if (fsize < progress) {
            std::filesystem::resize_file(task->dstPath, 0);
            wxTheApp->CallAfter([this, task, progress]() {
                TaskProgressUpdated(task, -(int64_t)progress);
            });
            progress = 0;
        }
    }

    OssSite *ossSite = (OssSite *)srcSite.get();
    size_t nparts = std::ceil((double)task->fileStat.size / SEGMENT);
    size_t n = std::ceil((double)progress / SEGMENT);
    std::shared_ptr<std::fstream> out(
            new std::fstream(task->dstPath, std::ios::binary | std::ios::app));
    for (; n < nparts; n++) {
        if (task->stop) {
            wxTheApp->CallAfter([this, task]() { TaskStopped(task); });
            return;
        }
        size_t offset = n * SEGMENT;
        size_t size = n < nparts - 1 ? SEGMENT : (task->fileStat.size - offset);
        Traffic traffic(Direction::Recv);
        Status status =
                ossSite->CopyFileToLocalPart(out,
                                             task->srcPath,
                                             task->offset + offset,
                                             task->offset + offset + size - 1);
        if (status.ok()) {
            wxTheApp->CallAfter(
                    [this, task, size]() { TaskProgressUpdated(task, size); });
        } else {
            wxTheApp->CallAfter(
                    [this, task, status]() { TaskFailed(task, status); });
            return;
        }
    }

    if (task->type == TTCopy) {
        LocalSite::SetLastModifiedTime(task->dstPath,
                                       task->fileStat.lastModifiedTime);
    }

    wxTheApp->CallAfter([this, task]() { TaskFinished(task, 0); });
}

void TaskList::ExecuteCopyPart(const TaskPtr &task,
                               const SitePtr &srcSite,
                               const SitePtr &dstSite) {
    if (srcSite->type() == STLocal) {
        ExecuteCopyFromLocalSegments(task, task->uploadId, srcSite, dstSite);
    } else {
        ExecuteCopyToLocalSegments(task, srcSite, dstSite);
    }
}

void TaskList::ExecuteCopyFinish(const TaskPtr &task) {
    if (task->stop) {
        wxTheApp->CallAfter([this, task]() { TaskStopped(task); });
        return;
    }

    SitePtr srcSite = createSite(task->srcSite);
    SitePtr dstSite = createSite(task->dstSite);
    if (!srcSite->IsOk() || !dstSite->IsOk()) {
        wxTheApp->CallAfter(
                [this, task]() { TaskFailed(task, Status(EC_FAIL, "")); });
        return;
    }

    Status status;
    if (srcSite->type() == STLocal) {
        OssSite *ossSite = (OssSite *)dstSite.get();
        status = ossSite->CopyFileFromLocalPartFinish(task->dstPath,
                                                      task->uploadId);
    } else {
        LocalSite *localSite = (LocalSite *)dstSite.get();
        status = localSite->ConcatParts(task->dstPath, task->children.size());
        if (status.ok()) {
            LocalSite::SetLastModifiedTime(task->dstPath,
                                           task->fileStat.lastModifiedTime);
        }
    }
    if (status.ok()) {
        wxTheApp->CallAfter([this, task, status]() { TaskFinished(task); });
    } else {
        wxTheApp->CallAfter(
                [this, task, status]() { TaskFailed(task, status); });
    }
}

void TaskList::ExecuteCopyAbort(const TaskPtr &task) {
    SitePtr dstSite = createSite(task->dstSite);
    if (dstSite->IsOk()) {
        OssSite *ossSite = (OssSite *)dstSite.get();
        auto [bucket, path] = ossSite->SplitPath(task->dstPath);
        ossSite->CopyFileFromLocalPartAbort(bucket, path, task->uploadId);
    }
}

void TaskList::TaskStarted(const TaskPtr &task) {
    task->status = TSStarted;
    task->startTime = std::time(nullptr);
    task->finishTime = 0;
    TaskUpdated(task);
}

void TaskList::TaskFinished(const TaskPtr &task, size_t progress, size_t size) {
    if (task->type == TTCopy) {
        auto [dstDirectory, _] = Site::Split(task->dstPath);
        directoryCenter()->NotifyDirectoryUpdated(task->dstSite, dstDirectory);
    }

    std::time_t tm = std::time(nullptr);
    TaskStatus taskStatus = TSFinished;
    task->status = taskStatus;
    task->finishTime = tm;
    int64_t amend = size == (size_t)-1 ? 0 : size - task->fileStat.size;

    task->progress += progress;
    task->fileStat.size += amend;
    TaskUpdated(task);

    TaskPtr parent = task;
    bool allDone = true;
    for (;;) {
        TaskPtr up = parent->parent.lock();
        if (up->type == TTSite) {
            break;
        }
        parent = up;
        parent->progress += progress;
        parent->fileStat.size += amend;

        if (allDone) {
            for (const auto &st : parent->children) {
                if (st->status < TSFinished) {
                    allDone = false;
                    break;
                }
                if (taskStatus < st->status) {
                    taskStatus = st->status;
                }
            }
            if (allDone) {
                if (taskStatus == TSFinished && task->type == TTCopyPart &&
                    task->parent.lock() == parent && parent->type == TTCopy) {
                    SubmitCopyFinish(parent);
                    break; // The TTCopy task have not finish yet.
                } else {
                    parent->status = taskStatus;
                    parent->finishTime = tm;
                }
            }
        }
        TaskUpdated(parent);
    }

    if (parent->scheduleId && parent->status >= TSFinished) {
        scheduleList()->Finished(parent->scheduleId);
    }

    bool removeFinishedTasks = options().get_bool(OPTION_DELETE_FINISHEDTASKS);
    if (removeFinishedTasks && parent->status == TSFinished) {
        RemoveTask(parent);
    }
}

void TaskList::TaskStopped(const TaskPtr &task) {
    std::time_t tm = std::time(nullptr);
    task->status = TSStopped;
    task->stop = false;
    task->finishTime = tm;
    TaskUpdated(task);

    TaskPtr parent = task;
    bool allDone = true;
    bool hasFailed = false;
    for (;;) {
        TaskPtr up = parent->parent.lock();
        if (up->type == TTSite) {
            break;
        }
        parent = up;
        if (allDone) {
            for (const auto &st : parent->children) {
                if (st->status < TSFinished) {
                    allDone = false;
                    break;
                }
                if (st->status == TSFailed) {
                    hasFailed = true;
                }
            }
        }
        if (allDone) {
            parent->status = hasFailed ? TSFailed : TSStopped;
            parent->finishTime = tm;
            TaskUpdated(parent);
        } else {
            break;
        }
    }

    if (parent->scheduleId && parent->status >= TSFinished) {
        scheduleList()->Finished(parent->scheduleId);
    }
}

void TaskList::TaskFailed(const TaskPtr &task, const Status &status) {
    std::time_t tm = std::time(nullptr);
    task->status = TSFailed;
    task->finishTime = tm;
    TaskUpdated(task);

    TaskPtr parent = task;
    bool allDone = true;
    for (;;) {
        TaskPtr up = parent->parent.lock();
        if (up->type == TTSite) {
            break;
        }
        parent = up;
        if (allDone) {
            for (const auto &st : parent->children) {
                if (st->status < TSFinished) {
                    allDone = false;
                    break;
                }
            }
        }
        if (allDone) {
            parent->status = TSFailed;
            parent->finishTime = tm;
            TaskUpdated(parent);
        } else {
            break;
        }
    }

    if (parent->scheduleId && parent->status >= TSFinished) {
        scheduleList()->Finished(parent->scheduleId);
    }
}

void TaskList::TaskProgressInit(const TaskPtr &task,
                                const std::string &uploadId,
                                size_t offset,
                                size_t size) {
    task->uploadId = uploadId;
    task->offset = offset;
    if (size != (size_t)-1) {
        task->fileStat.size = size;
    }
    TaskUpdated(task);
}

void TaskList::TaskProgressUpdated(const TaskPtr &task, int64_t progress) {
    task->progress += progress;
    TaskUpdated(task);

    for (TaskPtr parent = task->parent.lock(); parent && parent->type != TTSite;
         parent = parent->parent.lock()) {
        parent->progress += progress;
        TaskUpdated(parent);
    }
}

void TaskList::TaskAdded(const TaskPtr &parent, const TaskPtr &task) {
    storage()->AppendTask(task);
    for (auto *l : listeners_) {
        l->TaskAdded(parent, task);
    }
}

void TaskList::TaskAdded(const TaskPtr &parent, const TaskPtrVec &tasks) {
    for (const auto &task : tasks) {
        storage()->AppendTask(task);
    }
    for (auto *l : listeners_) {
        l->TaskAdded(parent, tasks);
    }
}

void TaskList::TaskUpdated(const TaskPtr &task) {
    storage()->UpdateTask(task);
    for (auto *l : listeners_) {
        l->TaskUpdated(task.get());
    }
}

void TaskList::TaskRemoved(const TaskPtr &task) {
    storage()->RemoveTask(task);

    for (auto *l : listeners_) {
        l->TaskRemoved(task.get());
    }
}

void TaskList::RemoveFinishedTasks() {
    TaskPtrVec finishedTasks;
    for (const auto &site : root_->children) {
        for (const auto &task : site->children) {
            if (task->status == TSFinished) {
                finishedTasks.push_back(task);
            }
        }
    }
    for (const auto &task : finishedTasks) {
        RemoveTask(task);
    }
}

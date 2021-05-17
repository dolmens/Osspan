#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "executor.h"
#include "local_site.h"
#include "oss_site.h"

enum TaskType {
    TTRoot,
    TTSite,
    TTSync,
    TTCopy,
    TTCopyPart,
};

enum TaskStatus {
    TSNone,
    TSPending,
    TSStarted,
    TSFinished,
    TSStopped,
    TSFailed,
};

struct Task;
using TaskPtr = std::shared_ptr<Task>;
using TaskWeakPtr = std::weak_ptr<Task>;
using TaskPtrVec = std::vector<TaskPtr>;
using TaskVecIter = TaskPtrVec::iterator;

#define SYNC_ONLY_NEW (1 << 0)
#define SYNC_UP (1 << 1)
#define SYNC_DOWN (1 << 2)

/**
 * Task里面的字段都是在主线程里面修改，后台线程读取。
 * 比如progress字段，任务新建或者从数据库加载并提交到后台执行时，后台会读取一次
 * 来计算偏移量，之后在执行过程中就不再读取并持续post到主线程去更新。
 * 特别要注意children vector，目前的设计在进入TTCopyFinishing后，执行线程
 * 会读取遍历children，所以，主线程在修改Task chidren变量时要先检查status，
 * 在TTCopyFinising状态下，主线程修改children不安全。
 * 多个线程都可能修改status，所以使用atomic。
 */
struct Task : public std::enable_shared_from_this<Task> {
    long id{0};
    long parentId{0};
    long scheduleId{0};
    TaskType type;
    TaskStatus status;
    std::atomic<bool> stop{false};
    std::string srcSite;
    std::string srcPath;
    std::string dstSite;
    std::string dstPath;
    FileStat fileStat;
    int flag{0};
    std::string uploadId;
    size_t offset{0};
    size_t partId{0};
    size_t progress{0};
    std::time_t startTime{0};
    std::time_t finishTime{0};

    TaskPtrVec children;
    TaskWeakPtr parent;
};

enum TaskRoutine {
    TROnce,
    TRRoutine,
};

enum TaskInterval {
    TIOnce,
    TIPerDay,
    TIPerHour,
    TIPerMinute,
};

struct TaskListListener {
    virtual void TaskAdded(const TaskPtr &parent, const TaskPtr &task) = 0;
    virtual void TaskAdded(const TaskPtr &parent, const TaskPtrVec &tasks) = 0;
    virtual void TaskUpdated(Task *task) = 0;
    virtual void TaskRemoved(Task *task) = 0;
};

class TaskList {
public:
    TaskList();
    ~TaskList();
    void Attach(TaskListListener *listener);
    void Detach(TaskListListener *listener);

    void AddTask(TaskType type,
                 const std::string &srcSite,
                 const std::string &srcPath,
                 const std::string &dstSite,
                 const std::string &dstPath,
                 const FileStat &stat = {},
                 int flag = (SYNC_UP | SYNC_DOWN));

    void AddTask(TaskType type,
                 long scheduleId,
                 const std::string &srcSite,
                 const std::string &srcPath,
                 const std::string &dstSite,
                 const std::string &dstPath,
                 const FileStat &stat = {},
                 int flag = (SYNC_UP | SYNC_DOWN));

    void AddTask(const TaskPtr &parent, const TaskPtr &task);

    void AddTask(const TaskPtr &parent, const TaskPtrVec &tasks);

    const TaskPtr &GetOssTask(const std::string &ossSiteName);

    // Called in worker threads to inform ui
    void TaskStarted(const TaskPtr &task);

    // Called in worker threads to inform ui
    void TaskFinished(const TaskPtr &task,
                      size_t progress = 0,
                      size_t size = (size_t)-1);

    void TaskStopped(const TaskPtr &task);

    void TaskFailed(const TaskPtr &task, const Status &status);

    void TaskProgressInit(const TaskPtr &task,
                          const std::string &uploadId,
                          size_t offset,
                          size_t size = (size_t)-1);

    void TaskProgressUpdated(const TaskPtr &task, int64_t progress);

    void StopTask(const TaskPtr &task);

    void ResumeTask(const TaskPtr &task);

    void RestartTask(const TaskPtr &task);

    void RemoveTask(const TaskPtr &task);

    void Load();
    void Close();

    const TaskPtr &root() { return root_; }

    void RemoveFinishedTasks();

protected:
    void Submit(const TaskPtr &task);
    void Submit(const TaskPtrVec &tasks);
    void SubmitCopyFinish(const TaskPtr &task);
    void SubmitCopyAbort(const TaskPtr &task);
    void Execute(const TaskPtr &task);
    void ExecuteSync(const TaskPtr &task,
                     const SitePtr &srcSite,
                     const SitePtr &dstSite);
    void ExecuteCopy(const TaskPtr &task,
                     const SitePtr &srcSite,
                     const SitePtr &dstSite);
    void ExecuteCopyDir(const TaskPtr &task,
                        const SitePtr &srcSite,
                        const SitePtr &dstSite);
    void ExecuteCopyBig(const TaskPtr &task,
                        const SitePtr &srcSite,
                        const SitePtr &dstSite);
    void ExecuteCopyPart(const TaskPtr &task,
                         const SitePtr &srcSite,
                         const SitePtr &dstSite);
    void ExecuteCopyFromLocalSegments(const TaskPtr &task,
                                      const std::string &uploadId,
                                      const SitePtr &srcSite,
                                      const SitePtr &dstSite);
    void ExecuteCopyToLocalSegments(const TaskPtr &task,
                                    const SitePtr &srcSite,
                                    const SitePtr &dstSite);
    void ExecuteCopyFinish(const TaskPtr &task);
    void ExecuteCopyAbort(const TaskPtr &task);

    void TaskAdded(const TaskPtr &parent, const TaskPtr &child);
    void TaskAdded(const TaskPtr &parent, const TaskPtrVec &childs);
    void TaskUpdated(const TaskPtr &task);
    void TaskRemoved(const TaskPtr &Task);

private:
    std::shared_ptr<Executor> tpDownload_;
    std::shared_ptr<Executor> tpUpload_;

    TaskPtr root_;
    std::vector<TaskListListener *> listeners_;
};

TaskList *taskList();

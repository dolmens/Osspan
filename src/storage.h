#pragma once

#include "task_list.h"
#include "schedule_list.h"
#include "oss_site_config.h"

#include <memory>

class Storage {
public:
    Storage();
    ~Storage();

    OssSiteNodePtrVec LoadSites();
    void AppendSite(OssSiteNodePtr &site);
    void UpdateSite(const OssSiteNodePtr &site);
    void RemoveSite(const OssSiteNodePtr &site);

    SchedulePtrVec LoadSchedules();
    void AppendSchedule(const SchedulePtr &schedule);
    void UpdateSchedule(const SchedulePtr &schedule);
    void RemoveSchedule(const SchedulePtr &schedule);

    TaskPtrVec LoadTasks();
    void AppendTask(const TaskPtr &task);
    void UpdateTask(const TaskPtr &task);
    // Will remove sub recursively
    void RemoveTask(const TaskPtr &task);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

Storage *storage();

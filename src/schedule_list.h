#pragma once

#include <ctime>
#include <string>
#include <thread>
#include <vector>

#include "status.h"
#include "task_list.h"

enum ScheduleRoutine {
    RTOnce,
    RTPerDay,
    RTPerHour,
    RTPerMinute,
};

std::wstring ScheduleRoutineToWString(ScheduleRoutine routine);

struct Schedule {
    long id{0};
    std::string name;
    std::string srcSite;
    std::string srcPath;
    std::string dstSite;
    std::string dstPath;
    int flag{SYNC_UP|SYNC_DOWN};
    ScheduleRoutine routine;
    std::time_t lastStartTime{0};
    std::time_t lastFinishTime{0};
};

using SchedulePtr = std::shared_ptr<Schedule>;
using SchedulePtrVec = std::vector<SchedulePtr>;

class ScheduleList {
public:
    ScheduleList();
    ~ScheduleList();

    void Schedule();

    Status Add(SchedulePtr &schedule);
    void Update(const SchedulePtr &schedule);
    void Remove(const SchedulePtr &schedule);
    void Remove(size_t index);
    const SchedulePtrVec &schedules() const { return schedules_; }

    void Finished(long scheduleId);

    std::string MakeTempName() const;
    bool NameExists(const std::string &name) const;

private:
    SchedulePtrVec schedules_;
};

ScheduleList *scheduleList();

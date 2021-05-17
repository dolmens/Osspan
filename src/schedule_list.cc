#include "schedule_list.h"
#include "storage.h"
#include "task_list.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "defs.h"
#include <wx/timer.h>

#include <set>
#include <ctime>

namespace {
class Looper : public wxTimer {
public:
    Looper(ScheduleList *scheduleList) : scheduleList_(scheduleList) {}

    void Notify() override { scheduleList_->Schedule(); }

private:
    wxTimer timer_;
    ScheduleList *scheduleList_;
};

} // namespace

std::wstring ScheduleRoutineToWString(ScheduleRoutine routine) {
    switch (routine) {
    case RTPerDay:
        return _("Per Day").ToStdWstring();
    case RTPerHour:
        return _("Per Hour").ToStdWstring();
    case RTPerMinute:
        return _("Per Minute").ToStdWstring();
    default:
        return L"";
    }
}

ScheduleList *scheduleList() {
    static ScheduleList schedules;
    return &schedules;
}

ScheduleList::ScheduleList() {
    static Looper looper(this);
    schedules_ = storage()->LoadSchedules();
    for (const auto &s : schedules_) {
        if (s->lastStartTime && s->lastFinishTime) {
            s->lastFinishTime = s->lastStartTime;
        }
    }
    wxTheApp->CallAfter([this]() { Schedule(); });
    looper.Start(60 * 1000);
}

ScheduleList::~ScheduleList() {
    for (const auto &s : schedules_) {
        storage()->UpdateSchedule(s);
    }
}

void ScheduleList::Schedule() {
    constexpr const std::time_t oneminute = 1 * 60;
    constexpr const std::time_t onehour = 60 * oneminute;
    constexpr const std::time_t oneday = 24 * onehour;

    std::time_t now = std::time(nullptr);
    for (const auto &s : schedules_) {
        if (!s->lastStartTime || s->lastFinishTime) {
            std::time_t interval = s->routine == RTPerDay    ? oneday
                                   : s->routine == RTPerHour ? onehour
                                                             : oneminute;
            if (now - s->lastFinishTime >= interval) {
                s->lastStartTime = now;
                s->lastFinishTime = 0;
                taskList()->AddTask(TTSync,
                                    s->id,
                                    s->srcSite,
                                    s->srcPath,
                                    s->dstSite,
                                    s->dstPath);
            }
        }
    }
}

Status ScheduleList::Add(SchedulePtr &schedule) {
    if (schedule->name.empty()) {
        schedule->name = MakeTempName();
    } else {
        if (NameExists(schedule->name)) {
            return Status(EC_FAIL, "name exists");
        }
    }
    storage()->AppendSchedule(schedule);
    schedules_.push_back(schedule);

    return Status::OK();
}

void ScheduleList::Update(const SchedulePtr &schedule) {
}

void ScheduleList::Remove(const SchedulePtr &schedule) {
    storage()->RemoveSchedule(schedule);
    schedules_.erase(
            std::remove(schedules_.begin(), schedules_.end(), schedule),
            schedules_.end());
}

void ScheduleList::Remove(size_t index) {
    storage()->RemoveSchedule(schedules_[index]);
    schedules_.erase(schedules_.begin() + index);
}

void ScheduleList::Finished(long scheduleId) {
    for (const auto &s : schedules_) {
        if (s->id == scheduleId) {
            s->lastFinishTime = std::time(nullptr);
            break;
        }
    }
}

std::string ScheduleList::MakeTempName() const {
    std::set<std::string> names;
    for (const auto &s : schedules_) {
        names.insert(s->name);
    }
    wxString wxPrefix = _("task");
    std::string prefix = wxPrefix.ToStdString();
    for (int i = 1; true; i++) {
        std::string name = prefix + std::to_string(i);
        if (!names.count(name)) {
            return name;
        }
    }
}

bool ScheduleList::NameExists(const std::string &name) const {
    for (const auto &s : schedules_) {
        if (s->name == name) {
            return true;
        }
    }
    return false;
}

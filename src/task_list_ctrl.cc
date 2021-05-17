#include "task_list_ctrl.h"
#include "options.h"

#include <set>

enum {
    ID_RESUME = wxID_HIGHEST + 1,
    ID_RESTART,
    ID_AUTODELETEFINISHED,
};

// clang-format off
wxBEGIN_EVENT_TABLE(TaskListCtrl, wxDataViewCtrl)
EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, TaskListCtrl::OnContextMenu)
EVT_MENU(wxID_STOP, TaskListCtrl::OnStopTask)
EVT_MENU(ID_RESUME, TaskListCtrl::OnResumeTask)
EVT_MENU(ID_RESTART, TaskListCtrl::OnRestartTask)
EVT_MENU(wxID_DELETE, TaskListCtrl::OnRemoveTask)
EVT_MENU(ID_AUTODELETEFINISHED, TaskListCtrl::OnAutoDelete)
EVT_SIZE(TaskListCtrl::OnSize)
wxEND_EVENT_TABLE();
// clang-format on

TaskListCtrl::TaskListCtrl(wxWindow *parent,
                           wxWindowID winid,
                           const wxPoint &pos,
                           const wxSize &size)
    : wxDataViewCtrl(parent, winid, pos, size, wxDV_MULTIPLE) {
    AppendTextColumn(_("Source"), 0, wxDATAVIEW_CELL_INERT, 300, wxALIGN_LEFT);
    AppendTextColumn(
            _("Destination"), 1, wxDATAVIEW_CELL_INERT, 300, wxALIGN_LEFT);
    AppendTextColumn(_("Type"), 2, wxDATAVIEW_CELL_INERT, 40, wxALIGN_LEFT);
    AppendTextColumn(_("Status"), 3, wxDATAVIEW_CELL_INERT, 40, wxALIGN_LEFT);
    AppendProgressColumn(_("Progress"), 4, wxDATAVIEW_CELL_INERT, 80);
    AppendTextColumn(_("Start"), 5, wxDATAVIEW_CELL_INERT, 110, wxALIGN_LEFT);
    AppendTextColumn(_("Finish"), 6, wxDATAVIEW_CELL_INERT, 110, wxALIGN_LEFT);

    model_ = new TaskListViewModel;
    AssociateModel(model_);
}

TaskListCtrl::~TaskListCtrl() {
    model_->DecRef();
}

void TaskListCtrl::OnContextMenu(wxDataViewEvent &event) {
    wxDataViewItemArray items;
    int cnt = GetSelections(items);
    bool canStop, canResume, canRestart, canDelete;
    if (cnt < 1) {
        canStop = canResume = canRestart = canDelete = false;
    } else if (cnt > 1) {
        canStop = canResume = canRestart = canDelete = true;
    } else {
        Task *task = (Task *)items[0].GetID();
        if (task->type == TTSite) {
            canStop = canResume = canRestart = canDelete = true;
        } else {
            if (task->status < TSFinished) {
                canStop = true;
                canResume = canRestart = canDelete = false;
            } else {
                canStop = false;
                canResume = task->status == TSStopped;
                canRestart = canDelete = task->type != TTCopyPart;
            }
        }
    }

    wxMenu menu;
    menu.Append(wxID_STOP, _("Stop"))->Enable(canStop);
    menu.Append(ID_RESUME, _("Resume"))->Enable(canResume);
    menu.Append(ID_RESTART, _("Restart"))->Enable(canRestart);
    menu.Append(wxID_DELETE, _("Delete"))->Enable(canDelete);
    menu.AppendSeparator();
    bool removeFinishedTasks = options().get_bool(OPTION_DELETE_FINISHEDTASKS);
    menu.AppendCheckItem(ID_AUTODELETEFINISHED, _("Auto Delete Finished Tasks"))
            ->Check(removeFinishedTasks);

    PopupMenu(&menu);
}

void TaskListCtrl::OnStopTask(wxCommandEvent &event) {
    std::vector<Task *> tasks = GetTopLevelSelectedTasks();
    for (const auto &t : tasks) {
        taskList()->StopTask(t->shared_from_this());
    }
}

void TaskListCtrl::OnResumeTask(wxCommandEvent &event) {
    std::vector<Task *> tasks = GetTopLevelSelectedTasks();
    for (const auto &t : tasks) {
        taskList()->ResumeTask(t->shared_from_this());
    }
}

void TaskListCtrl::OnRestartTask(wxCommandEvent &event) {
    std::vector<Task *> tasks = GetTopLevelSelectedTasks();
    for (const auto &t : tasks) {
        taskList()->RestartTask(t->shared_from_this());
    }
}

void TaskListCtrl::OnRemoveTask(wxCommandEvent &event) {
    std::vector<Task *> tasks = GetTopLevelSelectedTasks(false);
    for (const auto &t : tasks) {
        taskList()->RemoveTask(t->shared_from_this());
    }
}

void TaskListCtrl::OnAutoDelete(wxCommandEvent &event) {
    bool removeFinishedTasks = options().get_bool(OPTION_DELETE_FINISHEDTASKS);
    removeFinishedTasks = !removeFinishedTasks;
    options().set(OPTION_DELETE_FINISHEDTASKS, removeFinishedTasks);
    if (removeFinishedTasks) {
        taskList()->RemoveFinishedTasks();
    }
}

void TaskListCtrl::OnSize(wxSizeEvent &event) {
    event.Skip();
}

std::vector<Task *> TaskListCtrl::GetSelectedTasks() {
    std::vector<Task *> tasks;
    wxDataViewItemArray items;
    int cnt = GetSelections(items);
    for (int i = 0; i < cnt; i++) {
        tasks.push_back(static_cast<Task *>(items[i].GetID()));
    }
    return tasks;
}

std::vector<Task *> TaskListCtrl::GetTopLevelSelectedTasks(bool excludeSite) {
    std::vector<Task *> tasks;
    std::set<Task *> seen;
    wxDataViewItemArray items;
    int cnt = GetSelections(items);
    for (int i = 0; i < cnt; i++) {
        Task *task = static_cast<Task *>(items[i].GetID());
        if (task) {
            if (excludeSite && task->type == TTSite) {
                for (const auto &t : task->children) {
                    if (!seen.count(t.get())) {
                        seen.insert(t.get());
                        tasks.push_back(t.get());
                    }
                }
            } else {
                bool hasSeen = false;
                for (Task *parent = task; parent;
                     parent = parent->parent.lock().get()) {
                    if (seen.count(parent)) {
                        hasSeen = true;
                        break;
                    }
                }
                if (!hasSeen) {
                    seen.insert(task);
                    tasks.push_back(task);
                }
            }
        }
    }
    return tasks;
}

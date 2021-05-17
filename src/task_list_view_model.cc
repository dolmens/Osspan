#include "task_list_view_model.h"
#include "string_format.h"

namespace {
const wxString &TaskStatusToString(TaskStatus status) {
    const static std::vector<wxString> names{
            "",
            _("Queueing"),
            _("Running"),
            _("Finish"),
            _("Stop"),
            _("Fail"),
    };
    return names[(size_t)status];
}
} // namespace

TaskListViewModel::TaskListViewModel() {
    TaskList *taskList = ::taskList();
    root_ = taskList->root();
    taskList->Attach(this);
}

void TaskListViewModel::GetValue(wxVariant &variant,
                                 const wxDataViewItem &item,
                                 unsigned int col) const {
    auto *task = (Task *)item.GetID();
    switch (col) {
    case 0:
        variant = task->type == TTSite ? task->srcSite : task->srcPath;
        break;
    case 1:
        variant = task->dstPath;
        break;
    case 2:
        variant = (task->type == TTSync ? _("Sync") : _("Copy"));
        break;
    case 3:
        variant = TaskStatusToString(task->status);
        break;
    case 4: {
        if (task->status == TSFinished) {
            variant = 100L;
        } else if (task->status >= TSStarted) {
            if (task->fileStat.size == 0) {
                variant = 10L;
            } else {
                long v = task->progress * 100.0 / task->fileStat.size;
                if (v < 10) {
                    v = 10;
                }
                variant = v;
            }
        } else {
            variant = 0L;
        }
    } break;
    case 5:
        variant = opstrftimew(task->startTime);
        break;
    case 6:
        variant = opstrftimew(task->finishTime);
        break;
    default:
        assert(0);
        break;
    }
}

bool TaskListViewModel::SetValue(const wxVariant &variant,
                                 const wxDataViewItem &item,
                                 unsigned int col) {
    return true;
}

wxDataViewItem TaskListViewModel::GetParent(const wxDataViewItem &item) const {
    auto *task = (Task *)item.GetID();

    if (!task) {
        return wxDataViewItem(nullptr);
    }

    if (task->parent.lock().get() == root_.get()) {
        return wxDataViewItem(nullptr);
    }

    return wxDataViewItem(task->parent.lock().get());
}

bool TaskListViewModel::IsContainer(const wxDataViewItem &item) const {
    auto *task = (Task *)item.GetID();
    return task->type <= TTSite || !task->children.empty();
}

bool TaskListViewModel::HasContainerColumns(const wxDataViewItem &item) const {
    auto *task = (Task *)item.GetID();
    return task->type > TTSite;
}

unsigned int TaskListViewModel::GetChildren(const wxDataViewItem &parent,
                                            wxDataViewItemArray &array) const {
    auto *task = (Task *)parent.GetID();

    if (!task) {
        task = root_.get();
    }

    for (const auto &child : task->children) {
        array.Add(wxDataViewItem(child.get()));
    }

    return task->children.size();
}

void TaskListViewModel::TaskAdded(const TaskPtr &parent, const TaskPtr &task) {
    auto *parentPointer = parent.get();
    if (parentPointer == root_.get()) {
        parentPointer = nullptr;
    }
    wxDataViewItem parentItem(parentPointer);
    wxDataViewItem childItem(task.get());
    ItemAdded(parentItem, childItem);
}

void TaskListViewModel::TaskAdded(const TaskPtr &parent,
                                  const TaskPtrVec &tasks) {
    wxDataViewItem parentItem(parent.get());
    wxDataViewItemArray childItems;
    ItemsAdded(parentItem, childItems);
}

void TaskListViewModel::TaskUpdated(Task *task) {
    wxDataViewItem item(task);
    ItemChanged(item);
}

void TaskListViewModel::TaskRemoved(Task *task) {
    wxDataViewItem parent(task->parent.lock().get());
    wxDataViewItem item(task);
    ItemDeleted(parent, item);
}

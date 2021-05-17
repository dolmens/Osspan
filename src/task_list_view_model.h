#pragma once

#include "task_list.h"

#include <wx/dataview.h>

class TaskListViewModel : public wxDataViewModel, TaskListListener {
public:
    TaskListViewModel();
    ~TaskListViewModel() {}

    unsigned int GetColumnCount() const override { return 2; }

    wxString GetColumnType(unsigned int col) const override { return "string"; }

    void GetValue(wxVariant &variant,
                  const wxDataViewItem &item,
                  unsigned int col) const override;

    bool SetValue(const wxVariant &variant,
                  const wxDataViewItem &item,
                  unsigned int col) override;

    wxDataViewItem GetParent(const wxDataViewItem &item) const override;

    bool IsContainer(const wxDataViewItem &item) const override;

    bool HasContainerColumns(const wxDataViewItem &item) const override;

    unsigned int GetChildren(const wxDataViewItem &parent,
                             wxDataViewItemArray &array) const override;

    void TaskAdded(const TaskPtr &parent, const TaskPtr &task) override;
    void TaskAdded(const TaskPtr &parent, const TaskPtrVec &tasks) override;
    void TaskUpdated(Task *task) override;
    void TaskRemoved(Task *task) override;

private:
    TaskPtr root_;
};

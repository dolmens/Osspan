#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/dataview.h>

#include "defs.h"
#include "task_list.h"
#include "task_list_view_model.h"

class TaskListCtrl : public wxDataViewCtrl {
public:
    TaskListCtrl(wxWindow *parent,
                 wxWindowID winid = wxID_ANY,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize);
    ~TaskListCtrl();

protected:
    void OnContextMenu(wxDataViewEvent &event);
    void OnStopTask(wxCommandEvent &event);
    void OnResumeTask(wxCommandEvent &event);
    void OnRestartTask(wxCommandEvent &event);
    void OnRemoveTask(wxCommandEvent &event);
    void OnAutoDelete(wxCommandEvent &event);

    void OnSize(wxSizeEvent &event);

    std::vector<Task *> GetSelectedTasks();
    std::vector<Task *> GetTopLevelSelectedTasks(bool excludeSite = true);

private:
    TaskListViewModel *model_;
    wxDECLARE_EVENT_TABLEex();
};

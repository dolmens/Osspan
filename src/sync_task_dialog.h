#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/filepicker.h>

#include "defs.h"
#include "dialogex.h"
#include "schedule_list_ctrl.h"

class SyncTaskDialog : public wxDialogEx {
public:
    SyncTaskDialog(wxWindow *parent, wxWindowID id);
    ~SyncTaskDialog() = default;

    void SetLocalPath(const std::string &localPath);
    void SetOssPath(const std::string &ossSite, const std::string &ossPath);

    void OnOssSiteChanged(wxCommandEvent& event);

    void ExpandScheduleList(bool expand = true);

    void OnBrowseOss(wxCommandEvent &event);
    void OnOk(wxCommandEvent &event);

private:
    void DisplaySchedule();
    bool CheckUpdate();
    bool SaveCurrent();

    void OnListButton(wxCommandEvent &event);
    void OnAdd(wxCommandEvent &event);
    void OnRemove(wxCommandEvent &event);
    void OnRadioButton(wxCommandEvent &event);

    void OnItemSelected(wxListEvent &event);

    void OnKeyDown(wxListEvent &event);

    SchedulePtr newSchedule_{new Schedule};
    SchedulePtr schedule_;

    wxButton *listButton_;
    wxButton *addButton_;
    wxButton *removeButton_;
    ScheduleListCtrl *scheduleListCtrl_;

    wxDirPickerCtrl *localPathPicker_;
    wxChoice *ossSiteChoice_;
    wxTextCtrl *ossPathText_;
    wxCheckBox *cbNewFilesOnly_;

    wxRadioButton *directionRadioDual_;
    wxRadioButton *directionRadioUp_;
    wxRadioButton *directionRadioDown_;
    wxRadioButton *routineRadioOnce_;
    wxRadioButton *routineRadioPerDay_;
    wxRadioButton *routineRadioPerHour_;
    wxRadioButton *routineRadioPerMinute_;

    wxDECLARE_EVENT_TABLEex();
};

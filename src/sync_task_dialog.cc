#include "sync_task_dialog.h"
#include "oss_browse_dialog.h"
#include "oss_site_config.h"
#include "schedule_list.h"
#include "task_list.h"

#include <wx/statline.h>

enum {
    ID_LISTBUTTON = wxID_HIGHEST + 1,
};

// clang-format off
wxBEGIN_EVENT_TABLE(SyncTaskDialog, wxDialogEx)
EVT_CHOICE(wxID_ANY, SyncTaskDialog::OnOssSiteChanged)
EVT_RADIOBUTTON(wxID_ANY, SyncTaskDialog::OnRadioButton)
EVT_BUTTON(ID_LISTBUTTON, SyncTaskDialog::OnListButton)
EVT_BUTTON(wxID_NEW, SyncTaskDialog::OnAdd)
EVT_BUTTON(wxID_DELETE, SyncTaskDialog::OnRemove)
EVT_LIST_ITEM_SELECTED(wxID_ANY, SyncTaskDialog::OnItemSelected)
EVT_BUTTON(wxID_OPEN, SyncTaskDialog::OnBrowseOss)
EVT_BUTTON(wxID_OK, SyncTaskDialog::OnOk)
EVT_LIST_KEY_DOWN(wxID_ANY, SyncTaskDialog::OnKeyDown)
wxEND_EVENT_TABLE();
// clang-format on

SyncTaskDialog::SyncTaskDialog(wxWindow *parent, wxWindowID id)
    : wxDialogEx(parent,
                 id,
                 _("sync task settings"),
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX) {
    const auto &lay = layout();
    auto main = lay.createMain(this);
    main->AddGrowableCol(0, 1);

    wxBoxSizer *listButtonRow = new wxBoxSizer(wxHORIZONTAL);
    main->Add(listButtonRow);
    listButton_ = new wxButton(this, ID_LISTBUTTON, _("Expand Routine Tasks"));
    listButtonRow->Add(listButton_);
    addButton_ = new wxButton(this, wxID_NEW, _("New"));
    listButtonRow->Add(addButton_);
    removeButton_ = new wxButton(this, wxID_DELETE, _("Delete"));
    listButtonRow->Add(removeButton_);

    scheduleListCtrl_ = new ScheduleListCtrl(this, wxID_ANY);
    main->Add(scheduleListCtrl_, 1, wxEXPAND);

    main->Add(new wxStaticLine(this), 1, wxEXPAND);

    auto *localPathRow = new wxBoxSizer(wxHORIZONTAL);
    main->Add(localPathRow, lay.grow);

    localPathRow->Add(new wxStaticText(this,
                                       wxID_ANY,
                                       _("Local Path"),
                                       wxDefaultPosition,
                                       wxSize(60, -1)),
                      lay.valign);
    localPathPicker_ = new wxDirPickerCtrl(this, wxID_ANY),
    localPathRow->Add(localPathPicker_, lay.grow)->SetProportion(1);

    auto *ossPathRow = new wxBoxSizer(wxHORIZONTAL);
    main->Add(ossPathRow, lay.grow);

    ossPathRow->Add(new wxStaticText(this,
                                     wxID_ANY,
                                     _T("OSS"),
                                     wxDefaultPosition,
                                     wxSize(60, -1)),
                    lay.valign);
    wxArrayString osses;
    for (const auto &ossSiteNode : ossSiteConfig()->sites) {
        osses.Add(ossSiteNode->name);
    }
    ossSiteChoice_ = new wxChoice(
            this, wxID_ANY, wxDefaultPosition, wxSize(120, -1), osses);
    ossPathRow->Add(ossSiteChoice_, lay.valigng)->SetProportion(1);
    ossPathRow->Add(new wxStaticText(this,
                                     wxID_ANY,
                                     _("Path"),
                                     wxDefaultPosition,
                                     wxSize(40, -1)),
                    lay.valign);
    ossPathText_ = new wxTextCtrl(this, wxID_ANY);
    ossPathRow->Add(ossPathText_, 4, wxEXPAND | wxRIGHT, 3);

    wxButton *ossBrowseBtn = new wxButton(this, wxID_OPEN, _("Browse"));
    ossPathRow->Add(ossBrowseBtn, 0, wxEXPAND | wxRIGHT, lay.gap);

    auto *directionRow = new wxBoxSizer(wxHORIZONTAL);
    main->Add(directionRow, 1, wxEXPAND);
    directionRadioDual_ = new wxRadioButton(this,
                                            wxID_ANY,
                                            _("Dual"),
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxRB_GROUP);
    directionRadioDual_->SetValue(true);
    directionRadioUp_ = new wxRadioButton(this, wxID_ANY, _("Upload Only"));
    directionRadioDown_ = new wxRadioButton(this, wxID_ANY, _("Download only"));
    directionRow->Add(directionRadioDual_);
    directionRow->Add(directionRadioUp_);
    directionRow->Add(directionRadioDown_);

    cbNewFilesOnly_ = new wxCheckBox(this, wxID_ANY, _("New files Only"));
    cbNewFilesOnly_->SetValue(false);
    main->Add(cbNewFilesOnly_);

    auto *routineRow = new wxBoxSizer(wxHORIZONTAL);
    main->Add(routineRow);
    routineRadioOnce_ = new wxRadioButton(this,
                                          wxID_ANY,
                                          _("Once"),
                                          wxDefaultPosition,
                                          wxDefaultSize,
                                          wxRB_GROUP);
    routineRadioOnce_->SetValue(true);
    routineRadioPerDay_ = new wxRadioButton(this, wxID_ANY, _("Per Day"));
    routineRadioPerHour_ = new wxRadioButton(this, wxID_ANY, _("Per Hour"));
    routineRadioPerMinute_ = new wxRadioButton(this, wxID_ANY, _("Per Minute"));

    routineRow->Add(routineRadioOnce_);
    routineRow->Add(routineRadioPerDay_);
    routineRow->Add(routineRadioPerHour_);
    routineRow->Add(routineRadioPerMinute_);

    wxSizer *buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttonSizer, 1, wxEXPAND);

    schedule_ = newSchedule_;

    SetClientSize(640, 400);
}

void SyncTaskDialog::SetLocalPath(const std::string &localPath) {
    newSchedule_->srcPath = localPath;
    localPathPicker_->SetPath(localPath);
    localPathPicker_->SetInitialDirectory(localPath);
}

void SyncTaskDialog::SetOssPath(const std::string &ossSite,
                                const std::string &ossPath) {
    newSchedule_->dstSite = ossSite;
    newSchedule_->dstPath = ossPath;
    ossSiteChoice_->SetStringSelection(ossSite);
    ossPathText_->SetValue(ossPath);
}

void SyncTaskDialog::DisplaySchedule() {
    localPathPicker_->SetPath(schedule_->srcPath);
    localPathPicker_->SetInitialDirectory(schedule_->srcPath);
    ossSiteChoice_->SetStringSelection(schedule_->dstSite);
    ossPathText_->SetValue(schedule_->dstPath);

    int flag = schedule_->flag;
    if ((flag & SYNC_UP) && (flag & SYNC_DOWN)) {
        directionRadioDual_->SetValue(true);
    } else if (flag & SYNC_UP) {
        directionRadioUp_->SetValue(true);
    } else {
        directionRadioDown_->SetValue(true);
    }
    cbNewFilesOnly_->SetValue(flag & SYNC_ONLY_NEW);

    switch (schedule_->routine) {
    case RTOnce:
        routineRadioOnce_->SetValue(true);
        break;
    case RTPerDay:
        routineRadioPerDay_->SetValue(true);
        break;
    case RTPerHour:
        routineRadioPerHour_->SetValue(true);
        break;
    case RTPerMinute:
        routineRadioPerMinute_->SetValue(true);
        break;
    }
}

bool SyncTaskDialog::CheckUpdate() {
    std::string localPath = localPathPicker_->GetPath().ToStdString();
    if (schedule_->srcPath != localPath) {
        return true;
    }

    std::string ossSite = ossSiteChoice_->GetStringSelection().ToStdString();
    if (schedule_->dstSite != ossSite) {
        return true;
    }

    std::string ossPath = ossPathText_->GetValue().ToStdString();
    if (schedule_->dstPath != ossPath) {
        return true;
    }

    int flag{0};
    if (cbNewFilesOnly_->GetValue()) {
        flag |= SYNC_ONLY_NEW;
    }
    flag |= directionRadioDual_->GetValue() ? (SYNC_UP | SYNC_DOWN)
            : directionRadioUp_->GetValue() ? SYNC_UP
                                            : SYNC_DOWN;
    if (schedule_->flag != flag) {
        return true;
    }
    ScheduleRoutine routine = routineRadioOnce_->GetValue()      ? RTOnce
                              : routineRadioPerDay_->GetValue()  ? RTPerDay
                              : routineRadioPerHour_->GetValue() ? RTPerHour
                                                                 : RTPerMinute;
    if (schedule_->routine != routine) {
        return true;
    }

    return false;
}

bool SyncTaskDialog::SaveCurrent() {
    wxString wxLocalPath = localPathPicker_->GetPath();
    if (wxLocalPath.empty()) {
        wxMessageBox(_("Please select local directory"), _("Error"));
        return false;
    }
    wxString wxOssPath = ossPathText_->GetValue();
    if (wxOssPath.empty() || wxOssPath.size() <= OSSPROTOPLEN ||
        !wxOssPath.CmpNoCase(_T("oss://"))) {
        wxMessageBox(_("Please select OSS directory"), _("Error"));
        return false;
    }
    int flag{0};
    if (cbNewFilesOnly_->GetValue()) {
        flag |= SYNC_ONLY_NEW;
    }
    flag |= directionRadioDual_->GetValue() ? (SYNC_UP | SYNC_DOWN)
            : directionRadioUp_->GetValue() ? SYNC_UP
                                            : SYNC_DOWN;

    ScheduleRoutine routine = routineRadioOnce_->GetValue()      ? RTOnce
                              : routineRadioPerDay_->GetValue()  ? RTPerDay
                              : routineRadioPerHour_->GetValue() ? RTPerHour
                                                                 : RTPerMinute;

    schedule_->srcPath = wxLocalPath.ToStdString();
    schedule_->dstSite = ossSiteChoice_->GetStringSelection().ToStdString();
    schedule_->dstPath = wxOssPath.ToStdString();
    schedule_->flag = flag;
    schedule_->routine = routine;

    if (schedule_ == newSchedule_) {
        if (schedule_->routine == RTOnce) {
            taskList()->AddTask(TTSync,
                                "",
                                schedule_->srcPath,
                                schedule_->dstSite,
                                schedule_->dstPath,
                                {0, 0},
                                flag);
        } else {
            scheduleList()->Add(schedule_);
        }
    } else {
        scheduleList()->Update(schedule_);
    }

    return true;
}

void SyncTaskDialog::OnListButton(wxCommandEvent &event) {
    if (scheduleListCtrl_->IsShown()) {
        ExpandScheduleList(false);
    } else {
        ExpandScheduleList(true);
    }
}

void SyncTaskDialog::OnAdd(wxCommandEvent &event) {
    int n = scheduleListCtrl_->GetItemCount();
    for (int i = 0; i < n; i++) {
        scheduleListCtrl_->SetItemState(i, 0, wxLIST_STATE_SELECTED);
    }
    schedule_ = newSchedule_;
    DisplaySchedule();
    routineRadioOnce_->Enable(true);
}

void SyncTaskDialog::OnRemove(wxCommandEvent &event) {
    scheduleListCtrl_->RemoveSelectedSchedules();
}

void SyncTaskDialog::OnItemSelected(wxListEvent &event) {
    long item = scheduleListCtrl_->GetNextItem(
            -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item != -1) {
        const SchedulePtr &schedule = scheduleList()->schedules()[item];
        if (schedule_ != schedule) {
            if (CheckUpdate()) {
                if (wxMessageBox(_("Schedule Changed, Want save?"),
                                 _("Hint Save"),
                                 wxYES_NO) == wxYES) {
                    if (!SaveCurrent()) {
                        // Reselect the old one.
                        for (int i = 0; i < scheduleListCtrl_->GetItemCount();
                             i++) {
                            if (scheduleList()->schedules()[i] == schedule_) {
                                scheduleListCtrl_->SetItemState(
                                        i,
                                        wxLIST_STATE_SELECTED,
                                        wxLIST_STATE_SELECTED);
                            } else {
                                scheduleListCtrl_->SetItemState(
                                        i, 0, wxLIST_STATE_SELECTED);
                            }
                        }
                        return;
                    }
                }
            }
            schedule_ = schedule;
            DisplaySchedule();
            routineRadioOnce_->Enable(false);
        }
    }
}

void SyncTaskDialog::OnRadioButton(wxCommandEvent &event) {
    wxObject *evtObj = event.GetEventObject();
}

void SyncTaskDialog::OnOssSiteChanged(wxCommandEvent &event) {
    ossPathText_->SetValue(_T("oss://"));
}

void SyncTaskDialog::ExpandScheduleList(bool expand) {
    if (expand) {
        listButton_->SetLabelText(_("Collapse Routine Tasks"));
        addButton_->Show(true);
        removeButton_->Show(true);
        scheduleListCtrl_->Show(true);
    } else {
        listButton_->SetLabelText(_("Expand Routine Tasks"));
        addButton_->Show(false);
        removeButton_->Show(false);
        scheduleListCtrl_->Show(false);
    }
    Layout();
}

void SyncTaskDialog::OnBrowseOss(wxCommandEvent &event) {
    OssBrowseDialog *ossBrowseDialog = new OssBrowseDialog(this, wxID_ANY);
    ossBrowseDialog->SetOssPath("OSS1", ossPathText_->GetValue().ToStdString());
    if (ossBrowseDialog->ShowModal() == wxID_OK) {
        ossPathText_->SetValue(ossBrowseDialog->GetOssPath());
    }

    ossBrowseDialog->Destroy();
}

void SyncTaskDialog::OnOk(wxCommandEvent &event) {
    if (SaveCurrent()) {
        EndModal(wxID_OK);
    }
}

void SyncTaskDialog::OnKeyDown(wxListEvent &event) {
    if (event.GetKeyCode() == WXK_RETURN) {
        wxCommandEvent event;
        OnOk(event);
        return;
    }
    event.Skip();
}

#include "traffic_setting_dialog.h"
#include "options.h"

// clang-format off
wxBEGIN_EVENT_TABLE(TrafficSettingDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, TrafficSettingDialog::OnOk)
wxEND_EVENT_TABLE();
// clang-format on

TrafficSettingDialog::TrafficSettingDialog(wxWindow *parent, wxWindowID id)
    : wxDialogEx(parent,
                 id,
                 _("traffic settings"),
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX) {
    const auto &lay = layout();
    auto main = lay.createMain(this, 1);
    main->AddGrowableCol(0);
    main->AddGrowableRow(0);

    auto form = lay.createFlex(3);
    form->AddGrowableCol(1);
    main->Add(form, 1, wxEXPAND);

    form->Add(new wxStaticText(this,
                               wxID_ANY,
                               _("Concurrent Download Count"),
                               wxDefaultPosition,
                               wxSize(180, -1)));
    int downloadThreadCount = options().get_int(OPTION_DOWNLOAD_THREAD_COUNT);
    downloadThreadsInput_ = new wxTextCtrl(
            this, wxID_ANY, wxString::Format(_T("%d"), downloadThreadCount));
    form->Add(downloadThreadsInput_, 1, wxEXPAND);
    form->Add(new wxStaticText(this, wxID_ANY, _T("")));

    form->Add(new wxStaticText(this,
                               wxID_ANY,
                               _("Concurrent Upload Count"),
                               wxDefaultPosition,
                               wxSize(180, -1)));
    int uploadThreadCount = options().get_int(OPTION_UPLOAD_THREAD_COUNT);
    uploadThreadsInput_ = new wxTextCtrl(
            this, wxID_ANY, wxString::Format(_T("%d"), uploadThreadCount));
    form->Add(uploadThreadsInput_, 1, wxEXPAND);
    form->Add(new wxStaticText(this, wxID_ANY, _T("")));

    form->Add(new wxStaticText(this,
                               wxID_ANY,
                               _("Download Speed Limit(KB)"),
                               wxDefaultPosition,
                               wxSize(180, -1)));
    int downloadSpeedLimit = options().get_int(OPTION_DOWNLOAD_SPEED_LIMIT);
    downloadSpeedLimitInput_ = new wxTextCtrl(
            this, wxID_ANY, wxString::Format(_T("%d"), downloadSpeedLimit));
    form->Add(downloadSpeedLimitInput_, 1, wxEXPAND);
    downloadSpeedEnableBox_ = new wxCheckBox(this, wxID_ANY, _T(""));
    bool downloadSpeedEnable = options().get_bool(OPTION_DOWNLOAD_SPEED_ENABLE);
    downloadSpeedEnableBox_->SetValue(downloadSpeedEnable);
    form->Add(downloadSpeedEnableBox_);

    form->Add(new wxStaticText(this,
                               wxID_ANY,
                               _("Upload Speed Limit(KB)"),
                               wxDefaultPosition,
                               wxSize(180, -1)));
    int uploadSpeedLimit = options().get_int(OPTION_UPLOAD_SPEED_LIMIT);
    uploadSpeedLimitInput_ = new wxTextCtrl(
            this, wxID_ANY, wxString::Format(_T("%d"), uploadSpeedLimit));
    form->Add(uploadSpeedLimitInput_, 1, wxEXPAND);
    uploadSpeedEnableBox_ = new wxCheckBox(this, wxID_ANY, _T(""));
    bool uploadSpeedEnable = options().get_bool(OPTION_UPLOAD_SPEED_ENABLE);
    uploadSpeedEnableBox_->SetValue(uploadSpeedEnable);
    form->Add(uploadSpeedEnableBox_);

    wxSizer *buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttonSizer, 1, wxEXPAND);

    // SetClientSize(wxSize(768, 360));
}

void TrafficSettingDialog::OnOk(wxCommandEvent &event) {
    if (!downloadThreadsInput_->IsEmpty()) {
        long downloadThreadCount;
        if (!downloadThreadsInput_->GetValue().ToLong(&downloadThreadCount)) {
            wxMessageBox(_("Concurrent Download Count Input Error!"),
                         _("Error"));
            return;
        }
        options().set(OPTION_DOWNLOAD_THREAD_COUNT, (int)downloadThreadCount);
    }
    int uploadThreadCount = options().get_int(OPTION_UPLOAD_THREAD_COUNT);
    if (!uploadThreadsInput_->IsEmpty()) {
        long uploadThreadCount;
        if (!uploadThreadsInput_->GetValue().ToLong(&uploadThreadCount)) {
            wxMessageBox(_("Concurrent Upload Count Input Error!"), _("Error"));
            return;
        }
        options().set(OPTION_UPLOAD_THREAD_COUNT, (int)uploadThreadCount);
    }
    int downloadSpeedLimit = options().get_int(OPTION_DOWNLOAD_SPEED_LIMIT);
    if (!downloadSpeedLimitInput_->IsEmpty()) {
        long downloadSpeedLimit;
        if (!downloadSpeedLimitInput_->GetValue().ToLong(&downloadSpeedLimit)) {
            wxMessageBox(_("Download Speed Limit Input Error"), _("Error"));
            return;
        }
        if (downloadSpeedLimit > 0 && downloadSpeedLimit < 100) {
            wxMessageBox(_("Download Speed Limit Min Value Is 100"),
                         _("Error"));
            return;
        }
        options().set(OPTION_DOWNLOAD_SPEED_LIMIT, (int)downloadSpeedLimit);
    }
    bool downloadSpeedEnable = downloadSpeedEnableBox_->GetValue();
    options().set(OPTION_DOWNLOAD_SPEED_ENABLE, downloadSpeedEnable);
    int uploadSpeedLimit = options().get_int(OPTION_UPLOAD_SPEED_LIMIT);
    if (!uploadSpeedLimitInput_->IsEmpty()) {
        long uploadSpeedLimit;
        if (!uploadSpeedLimitInput_->GetValue().ToLong(&uploadSpeedLimit)) {
            wxMessageBox(_("Upload Speed Limit Input Error"), _("Error"));
            return;
        }
        if (uploadSpeedLimit > 0 && uploadSpeedLimit < 100) {
            wxMessageBox(_("Upload Speed Limit Min Value Is 100"), _("Error"));
            return;
        }
        options().set(OPTION_UPLOAD_SPEED_LIMIT, (int)uploadSpeedLimit);
    }
    bool uploadSpeedEnable = uploadSpeedEnableBox_->GetValue();
    options().set(OPTION_UPLOAD_SPEED_ENABLE, uploadSpeedEnable);

    EndModal(wxID_OK);
}

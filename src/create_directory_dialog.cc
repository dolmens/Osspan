#include "create_directory_dialog.h"
#include "global_executor.h"

// clang-format off
wxBEGIN_EVENT_TABLE(CreateDirectoryDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, CreateDirectoryDialog::OnOk)
EVT_BUTTON(wxID_CANCEL, CreateDirectoryDialog::OnCancel)
wxEND_EVENT_TABLE();
// clang-format on

CreateDirectoryDialog::CreateDirectoryDialog(Site *site, wxWindow *parent)
    : wxDialogEx(parent, wxID_ANY, "创建目录") {
    site_ = site;
    auto &lay = layout();
    auto *main = lay.createMain(this);
    main->Add(new wxStaticText(this, wxID_ANY, "请输入要创建的目录名称:"));
    textCtrl_ = new wxTextCtrl(this, wxID_ANY);
    main->Add(textCtrl_, lay.grow)->SetMinSize(lay.dlgUnits(150), -1);

    auto *buttons = lay.createButtonSizer(this, main, true);

    auto ok = new wxButton(this, wxID_OK, "OK");
    ok->SetDefault();
    buttons->AddButton(ok);

    auto cancel = new wxButton(this, wxID_CANCEL, "Cancel");
    buttons->AddButton(cancel);
    buttons->Realize();

    GetSizer()->Fit(this);

    textCtrl_->SetFocus();
    ok->Disable();

    textCtrl_->Bind(wxEVT_TEXT, [this, ok](const wxEvent &) {
        ok->Enable(!textCtrl_->GetValue().empty());
    });
}

void CreateDirectoryDialog::OnOk(wxCommandEvent &event) {
    globalExecutor()->submit([this]() {
        std::string name = textCtrl_->GetValue().ToStdString();
        Status status =
                site_->MakeDir(Site::Combine(site_->GetCurrentPath(), name));
        wxTheApp->CallAfter([this, status]() {
            if (status.ok()) {
                site_->Refresh(true,
                               [this](Site *, Status) { EndModal(wxID_OK); });
            }
        });
    });
}

void CreateDirectoryDialog::OnCancel(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}

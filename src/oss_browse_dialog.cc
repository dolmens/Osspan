#include "oss_browse_dialog.h"
#include "oss_client.h"

// clang-format off
wxBEGIN_EVENT_TABLE(OssBrowseDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, OssBrowseDialog::OnOk)
wxEND_EVENT_TABLE();
// clang-format on

OssBrowseDialog::OssBrowseDialog(wxWindow *parent, wxWindowID id)
    : wxDialogEx(parent,
                 id,
                 _("Choose OSS Directory"),
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX) {
    const auto &lay = layout();
    auto main = lay.createMain(this);
    main->AddGrowableCol(0);
    main->AddGrowableRow(0);

    ossListView_ = new OssListView(this);
    main->Add(ossListView_, 1, wxEXPAND);

    wxSizer *buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttonSizer, 1, wxEXPAND);

    SetClientSize(wxSize(640, 300));
}

void OssBrowseDialog::SetOssPath(const std::string &ossSiteName,
                                 const std::string &ossPath) {
    std::shared_ptr<OssSite> ossSite(new OssSite(ossSiteName));
    std::string path;
    if (ossPath.empty()) {
        path = OSSPROTOP;
    } else if (ossPath.front() == '/') {
        path = "oss:/" + ossPath;
    } else if (strncasecmp(ossPath.c_str(), OSSPROTOP, OSSPROTOPLEN)) {
        path = OSSPROTOP + ossPath;
    } else {
        path = ossPath;
    }
    ossSite->ChangeDir(path);
    ossListView_->GetUriBox()->ChangeValue(path);
    ossListView_->AttachSite(ossSite);
}

wxString OssBrowseDialog::GetOssPath() const {
    return ossListView_->GetListCtrl()->GetCurrentPath();
}

void OssBrowseDialog::OnOk(wxCommandEvent &event) {
    if (Validate()) {
        EndModal(wxID_OK);
    }
}

bool OssBrowseDialog::Validate() {
    if (ossListView_->GetListCtrl()->GetCurrentDirTag() == OssBucketsTag) {
        long item = ossListView_->GetListCtrl()->GetNextItem(
                item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1) {
            wxMessageBox(_("Please select directory"), _("Error"));
            return false;
        }
        const FilePtr &file =
                ossListView_->GetListCtrl()->GetSite()->GetFiles()[item];
        ossListView_->GetListCtrl()->GetSite()->ChangeToSubDir(file->name);
        return false;
    }

    return true;
}

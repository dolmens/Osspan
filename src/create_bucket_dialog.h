#pragma once

#include "defs.h"
#include "dialogex.h"
#include "oss_site.h"

class CreateBucketDialog : public wxDialogEx {
public:
    CreateBucketDialog(OssSite *site, wxWindow *parent);
    ~CreateBucketDialog() = default;

    void OnOk(wxCommandEvent &event);
    void Ok();

private:
    OssSite *site_;
    wxTextCtrl *nameInput_;
    wxComboBox *region_;
    wxComboBox *acl_;
    wxComboBox *storageClass_;

    wxDECLARE_EVENT_TABLEex();
};

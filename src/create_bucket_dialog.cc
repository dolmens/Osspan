#include "create_bucket_dialog.h"
#include "oss_regions.h"

#include "global_executor.h"
#include <thread>

// clang-format off
wxBEGIN_EVENT_TABLE(CreateBucketDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, CreateBucketDialog::OnOk)
wxEND_EVENT_TABLE();
// clang-format on

CreateBucketDialog::CreateBucketDialog(OssSite *site, wxWindow *parent)
    : wxDialogEx(parent,
                 wxID_ANY,
                 "创建Bucket",
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER | wxCLOSE_BOX),
      site_(site) {
    const auto &lay = layout();
    auto main = lay.createMain(this);
    main->AddGrowableCol(0);
    main->AddGrowableRow(0);

    auto form = lay.createFlex(2);
    main->Add(form, lay.grow)->SetProportion(1);
    form->AddGrowableCol(1);

    form->Add(new wxStaticText(this, wxID_ANY, _T("名称")));
    nameInput_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(nameInput_, lay.grow)->SetProportion(1);

    form->Add(new wxStaticText(this, wxID_ANY, _T("区域")));
    const std::vector<const char *> &regions = ossRegions();
    wxArrayString regionOptions(regions.size() - 1,
                                (const char **)(regions.data() + 1));
    region_ = new wxComboBox(this,
                             wxID_ANY,
                             wxEmptyString,
                             wxDefaultPosition,
                             wxDefaultSize,
                             regionOptions,
                             wxCB_READONLY);
    form->Add(region_, lay.grow)->SetProportion(1);

    form->Add(new wxStaticText(this, wxID_ANY, _T("ACL权限")));
    wxString aclOptionItems[] = {
            _T("私有"),
            _T("公共读"),
            _T("公共读写"),
    };
    acl_ = new wxComboBox(this,
                          wxID_ANY,
                          wxEmptyString,
                          wxDefaultPosition,
                          wxDefaultSize,
                          std::size(aclOptionItems),
                          aclOptionItems,
                          wxCB_READONLY);
    form->Add(acl_, lay.grow)->SetProportion(1);

    form->Add(new wxStaticText(this, wxID_ANY, _T("类型")));
    wxString storageItems[] = {
            _T("标准"),
            _T("低频访问"),
            _T("归档存储"),
    };
    storageClass_ = new wxComboBox(this,
                                   wxID_ANY,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   std::size(storageItems),
                                   storageItems,
                                   wxCB_READONLY);
    form->Add(storageClass_, lay.grow)->SetProportion(1);

    auto *buttons = lay.createButtonSizer(this, main, true);

    auto ok = new wxButton(this, wxID_OK, "OK");
    ok->SetDefault();
    buttons->AddButton(ok);
    auto cancel = new wxButton(this, wxID_CANCEL, "Cancel");
    buttons->AddButton(cancel);
    buttons->Realize();

    SetClientSize(wxSize(360, 200));

    nameInput_->SetFocus();
}

void CreateBucketDialog::OnOk(wxCommandEvent &event) {
    globalExecutor()->submit([this]() {
        std::string name = nameInput_->GetValue().ToStdString();
        const std::vector<const char *> &regionCodes = ossRegionCodes();
        std::string region = regionCodes[region_->GetSelection() + 1];
        oss::StorageClass storageClasses[] = {oss::StorageClass::Standard,
                                              oss::StorageClass::IA,
                                              oss::StorageClass::Archive};
        oss::StorageClass storageClass =
                storageClasses[storageClass_->GetSelection()];
        oss::CannedAccessControlList acls[] = {
                oss::CannedAccessControlList::Private,
                oss::CannedAccessControlList::PublicRead,
                oss::CannedAccessControlList::PublicReadWrite};
        oss::CannedAccessControlList acl = acls[acl_->GetSelection()];
        Status status = site_->CreateBucket(name, region, storageClass, acl);
        wxTheApp->CallAfter([this, status]() {
            if (status.ok()) {
                site_->Refresh(true, [this](Site *, Status) { this->Ok(); });
            }
        });
    });
}

void CreateBucketDialog::Ok() {
    this->EndModal(wxID_OK);
}

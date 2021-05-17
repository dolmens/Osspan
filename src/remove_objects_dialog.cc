#include "remove_objects_dialog.h"
#include "global_executor.h"
#include "theme_provider.h"

#include <wx/imaglist.h>

// clang-format off
wxBEGIN_EVENT_TABLE(RemoveObjectsListCtrl, wxListCtrl)
EVT_LIST_KEY_DOWN(wxID_ANY, RemoveObjectsListCtrl::OnKeyDown)
wxEND_EVENT_TABLE();
// clang-format on

// clang-format off
wxBEGIN_EVENT_TABLE(RemoveObjectsDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, RemoveObjectsDialog::OnOk)
EVT_BUTTON(wxID_CANCEL, RemoveObjectsDialog::OnCancel)
wxEND_EVENT_TABLE();
// clang-format on

// clang-format off
wxBEGIN_EVENT_TABLE(RemoveBucketsDialog, wxDialogEx)
EVT_BUTTON(wxID_OK, RemoveBucketsDialog::OnOk)
EVT_BUTTON(wxID_CANCEL, RemoveBucketsDialog::OnCancel)
wxEND_EVENT_TABLE();
// clang-format on

RemoveObjectsListCtrl::RemoveObjectsListCtrl(
        const std::vector<std::string> &items,
        RemoveObjectsDialogBase *parent)
    : wxListCtrl(parent,
                 wxID_ANY,
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxLC_REPORT | wxLC_VIRTUAL | wxLC_NO_HEADER),
      items_(items),
      status_(items.size(), -1),
      parent_(parent) {
    wxSize imageSize = themeProvider()->GetIconSize(iconSizeSmall);
    wxImageList *imageList = new wxImageList(imageSize.x, imageSize.y);
    wxBitmap cross =
            themeProvider()->CreateBitmap("ART_CROSS", wxART_OTHER, imageSize);
    wxBitmap check =
            themeProvider()->CreateBitmap("ART_CHECK", wxART_OTHER, imageSize);
    imageList->Add(cross);
    imageList->Add(check);
    SetImageList(imageList, wxIMAGE_LIST_SMALL);
    AppendColumn(_("Name"));
    SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
    SetItemCount(items_.size());
}

void RemoveObjectsListCtrl::Update(long item, bool status) {
    status_[item] = status;
    RefreshItem(item);
}

wxString RemoveObjectsListCtrl::OnGetItemText(long item, long column) const {
    return items_[item];
}

int RemoveObjectsListCtrl::OnGetItemImage(long item) const {
    return status_[item];
}

void RemoveObjectsListCtrl::OnKeyDown(wxListEvent &event) {
    if (event.GetKeyCode() == WXK_RETURN) {
        wxCommandEvent event;
        parent_->OnOk(event);
        return;
    }
    event.Skip();
}

RemoveObjectsDialog::RemoveObjectsDialog(Site *site,
                                         const std::vector<std::string> &items,
                                         wxWindow *parent)
    : RemoveObjectsDialogBase(parent,
                              wxID_ANY,
                              _("Delete Items"),
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER |
                                      wxCLOSE_BOX),
      site_(site),
      items_(items) {
    const auto &lay = layout();
    auto *main = lay.createMain(this);
    main->AddGrowableCol(0);
    main->AddGrowableRow(1);

    main->Add(new wxStaticText(
            this, wxID_ANY, _("Do you want to delete these items?")));

    listCtrl_ = new RemoveObjectsListCtrl(items_, this);
    main->Add(listCtrl_, lay.grow)->SetProportion(1);

    wxSizer *buttons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttons, lay.grow)->SetProportion(1);
    GetSizer()->Fit(this);
}

void RemoveObjectsDialog::OnOk(wxCommandEvent &event) {
    if (processing_) {
        return;
    }
    processing_ = true;
    total_ = items_.size();
    allOk_ = true;
    globalExecutor()->submit([this]() {
        for (size_t i = 0; i < items_.size(); i++) {
            if (cancel_) {
                break;
            }
            pending_++;
            const std::string &item = items_[i];
            Status status = site_->Remove(item);
            wxTheApp->CallAfter([this, i, status]() {
                Update(i, status.ok());
                site_->Refresh();
                allOk_ = allOk_ && status.ok();
                if (--pending_ == 0 && cancel_) {
                    EndModal(wxID_CANCEL);
                }
                if (--total_ == 0 && allOk_) {
                    EndModal(wxID_OK);
                }
            });
        }
    });
}

void RemoveObjectsDialog::OnCancel(wxCommandEvent &event) {
    if (!processing_ || total_ == 0) {
        EndModal(wxID_CANCEL);
    } else {
        cancel_ = true;
    }
}

void RemoveObjectsDialog::Update(long item, bool status) {
    listCtrl_->Update(item, status);
}

RemoveBucketsDialog::RemoveBucketsDialog(OssSite *site,
                                         const std::vector<std::string> &items,
                                         wxWindow *parent)
    : RemoveObjectsDialogBase(parent,
                              wxID_ANY,
                              "删除项目",
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER |
                                      wxCLOSE_BOX),
      site_(site),
      items_(items) {
    const auto &lay = layout();
    auto *main = lay.createMain(this);
    main->AddGrowableCol(0);
    main->AddGrowableRow(1);

    main->Add(new wxStaticText(this, wxID_ANY, "确认要删除如下项目吗？"));

    listCtrl_ = new RemoveObjectsListCtrl(items_, this);
    main->Add(listCtrl_, lay.grow)->SetProportion(1);

    wxSizer *buttons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    main->Add(buttons, lay.grow)->SetProportion(1);

    GetSizer()->Fit(this);
}

void RemoveBucketsDialog::OnOk(wxCommandEvent &event) {
    if (processing_) {
        return;
    }
    processing_ = true;
    total_ = items_.size();
    allOk_ = true;
    globalExecutor()->submit([this]() {
        for (size_t i = 0; i < items_.size(); i++) {
            if (cancel_) {
                break;
            }
            pending_++;
            const std::string &item = items_[i];
            Status status = site_->RemoveBucket(item);
            wxTheApp->CallAfter([this, i, status]() {
                Update(i, status.ok());
                site_->Refresh();
                allOk_ = allOk_ && status.ok();
                if (--pending_ == 0 && cancel_) {
                    EndModal(wxID_CANCEL);
                }
                if (--total_ == 0 && allOk_) {
                    EndModal(wxID_OK);
                }
            });
        }
    });
}

void RemoveBucketsDialog::OnCancel(wxCommandEvent &event) {
    if (!processing_ || total_ == 0) {
        EndModal(wxID_CANCEL);
    } else {
        cancel_ = true;
    }
}

void RemoveBucketsDialog::Update(long item, bool status) {
    listCtrl_->Update(item, status);
}

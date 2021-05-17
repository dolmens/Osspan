#include "uri_box.h"
#include "explorer.h"

#include <set>

// clang-format off
wxBEGIN_EVENT_TABLE(UriBox, wxComboBox)
EVT_COMBOBOX(wxID_ANY, UriBox::Go)
EVT_TEXT_ENTER(wxID_ANY, UriBox::Go)
EVT_KEY_DOWN(UriBox::OnKeyDown)
wxEND_EVENT_TABLE();
// clang-format on

UriBox::UriBox(wxWindow *parent)
    : wxComboBox(parent,
                 wxID_ANY,
                 wxEmptyString,
                 wxDefaultPosition,
                 wxDefaultSize,
                 wxArrayString(),
                 wxCB_DROPDOWN | wxTE_PROCESS_ENTER) {
}

void UriBox::OnKeyDown(wxKeyEvent &event) {
    if (event.GetKeyCode() == WXK_ESCAPE) {
        SetValue(site_->GetCurrentPath());
        return;
    }
    if (event.GetKeyCode() == WXK_TAB) {
        if (peer_) {
            peer_->SetFocus();
        }
        return;
    }
    event.Skip();
}

void UriBox::Go(wxCommandEvent &evt) {
    if (GetValue().empty()) {
        wxBell();
        return;
    }

    std::string path = Format();
    if (path.empty()) {
        wxBell();
        return;
    }

    if (explorer_) {
        explorer_->willChangeDir(site_.get(), path);
    }

    site_->ChangeDir(path, [this](Site *site, Status status) {
        if (!status.ok()) {
            wxBell();
        }
    });
}

void UriBox::SiteUpdated(Site *site) {
    Clear();
    std::set<std::string> seen;
    const auto &forwards = site_->GetForwards();
    const auto &backwards = site_->GetBackwards();
    for (const auto &dir : forwards) {
        if (!seen.count(dir->path)) {
            seen.insert(dir->path);
            Append(dir->path);
        }
    }
    for (const auto &dir : backwards) {
        if (!seen.count(dir->path)) {
            seen.insert(dir->path);
            Append(dir->path);
        }
    }

    SetValue(site_->GetCurrentPath());
}

#include "multi_tab_explorer.h"
#include "options.h"

// clang-format off
wxBEGIN_EVENT_TABLE(MultiTabExplorer, wxPanel)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, MultiTabExplorer::OnTabChanged)
EVT_AUINOTEBOOK_PAGE_CLOSED(wxID_ANY, MultiTabExplorer::OnTabClosed)
EVT_SPLITTER_UNSPLIT(wxID_ANY, MultiTabExplorer::OnUnsplit)
wxEND_EVENT_TABLE();
// clang-format on

MultiTabExplorer::MultiTabExplorer(wxWindow *parent,
                                   wxWindowID winid,
                                   const wxPoint &pos,
                                   const wxSize &size,
                                   long style,
                                   const wxString &name)
    : wxPanel(parent, winid, pos, size, style, name) {
    mainSizer_ = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer_);
}

MultiTabExplorer::~MultiTabExplorer() {
}

void MultiTabExplorer::Open(const std::string &ossSiteName) {
    if (!explorer_) {
        explorer_ = new Explorer(ossSiteName,
                                 this,
                                 wxID_ANY,
                                 wxDefaultPosition,
                                 GetClientSize());
        explorer_->AddListener(this);
        mainSizer_->Add(explorer_, 1, wxEXPAND);
        for (auto *l : listeners_) {
            l->ExplorerSwitched(explorer_);
        }
    } else {
        if (!tabs_) {
            tabs_ = new wxAuiNotebook(this);
        }
        if (tabs_->GetPageCount() == 0) {
            mainSizer_->Detach(explorer_);
            explorer_->Reparent(tabs_);
            tabs_->AddPage(explorer_, explorer_->GetOssSite()->name());
            mainSizer_->Add(tabs_, 1, wxEXPAND);
            Layout();
        }
        Explorer *explorer = new Explorer(ossSiteName, tabs_);
        explorer->AddListener(this);
        tabs_->AddPage(explorer, explorer->GetOssSite()->name(), true);
    }
}

void MultiTabExplorer::Clear() {
    if (tabs_) {
        if (explorer_ && explorer_->GetParent() != tabs_) {
            explorer_->Destroy();
        }
        explorer_ = nullptr;
        tabs_->Destroy();
        tabs_ = nullptr;
    } else if (explorer_) {
        explorer_->Destroy();
        explorer_ = nullptr;
    }
    options().set(OPTION_LASTOPEN_SITES, std::vector<std::string>());
}

void MultiTabExplorer::Up() {
    if (explorer_) {
        explorer_->Up();
    }
}

void MultiTabExplorer::Backward() {
    if (explorer_) {
        explorer_->Backward();
    }
}

void MultiTabExplorer::Forward() {
    if (explorer_) {
        explorer_->Forward();
    }
}

void MultiTabExplorer::RefreshFileItems() {
    if (explorer_) {
        explorer_->RefreshFileItems();
    }
}

void MultiTabExplorer::OnTwinMode() {
    if (explorer_) {
        explorer_->OnTwinMode();
    }
}

// 一次性比较，离开目录即失效
void MultiTabExplorer::OnDiff() {
    if (explorer_) {
        explorer_->OnDiff();
    }
}

// 当前选中文件拷贝
void MultiTabExplorer::Copy() {
    if (explorer_) {
        explorer_->Copy();
    }
}

void MultiTabExplorer::SetSelection(size_t index) {
    if (tabs_ && tabs_->GetPageCount() > 1) {
        tabs_->SetSelection(index);
    }
}

size_t MultiTabExplorer::GetTabCount() const {
    if (!tabs_) {
        return 1;
    }
    size_t pageCount = tabs_->GetPageCount();
    return pageCount ? pageCount : 1;
}

std::vector<std::string> MultiTabExplorer::GetOpenedSites() const {
    std::vector<std::string> sites;
    if (tabs_ && tabs_->GetPageCount()) {
        for (size_t i = 0; i < tabs_->GetPageCount(); i++) {
            sites.push_back(
                    static_cast<Explorer *>(tabs_->GetPage(i))->siteName());
        }
    } else if (explorer_) {
        sites.push_back(explorer_->siteName());
    }

    return sites;
}

int MultiTabExplorer::GetSelection() const {
    return tabs_ ? tabs_->GetSelection() : -1;
}

std::shared_ptr<LocalSite> MultiTabExplorer::GetCurrentLocalSite() {
    if (explorer_) {
        return explorer_->GetLocalSite();
    }
    return {};
}

std::shared_ptr<OssSite> MultiTabExplorer::GetCurrentOssSite() {
    if (explorer_) {
        return explorer_->GetOssSite();
    }
    return {};
}

void MultiTabExplorer::AdvanceTab(bool forward) {
    if (tabs_ && tabs_->GetPageCount() > 1) {
        tabs_->AdvanceSelection(forward);
    }
}

void MultiTabExplorer::AddListener(MultiTabExplorerListener *l) {
    if (std::find(listeners_.begin(), listeners_.end(), l) ==
        listeners_.end()) {
        listeners_.push_back(l);
    }
}

void MultiTabExplorer::RemoveListener(MultiTabExplorerListener *l) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), l),
                     listeners_.end());
}

void MultiTabExplorer::ExplorerSiteUpdated(Explorer *explorer, Site *site) {
    for (auto *l : listeners_) {
        l->ExplorerSiteUpdated(explorer, site);
    }
}

void MultiTabExplorer::ExplorerSiteSwitched(Explorer *explorer, Site *site) {
    for (auto *l : listeners_) {
        l->ExplorerSiteSwitched(explorer, site);
    }
}

void MultiTabExplorer::ExplorerSelectionUpdated(Explorer *explorer,
                                                FileListCtrl *fileListCtrl) {
    for (auto *l : listeners_) {
        l->ExplorerSelectionUpdated(explorer, fileListCtrl);
    }
}

void MultiTabExplorer::ExplorerDiffQuit(Explorer *explorer) {
    for (auto *l : listeners_) {
        l->ExplorerDiffQuit(explorer_);
    }
}

void MultiTabExplorer::OnTabChanged(wxAuiNotebookEvent &event) {
    explorer_ = (Explorer *)tabs_->GetCurrentPage();
    for (auto *l : listeners_) {
        l->ExplorerSwitched(explorer_);
    }
}

void MultiTabExplorer::OnTabClosed(wxAuiNotebookEvent &event) {
    if (tabs_->GetPageCount() == 1) {
        tabs_->RemovePage(0);
        explorer_->Reparent(this);
        mainSizer_->Remove(0);
        mainSizer_->Add(explorer_, 1, wxEXPAND);
        Layout();
    }
}

void MultiTabExplorer::OnUnsplit(wxSplitterEvent &event) {
    if (event.GetEventObject() == explorer_) {
        for (auto *l : listeners_) {
            l->TwinModeSwitched(explorer_, false);
        }
    }
}

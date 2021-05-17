#include "explorer.h"
#include "directory_compare.h"
#include "oss_site_config.h"

#include <wx/stdpaths.h>

Explorer::Explorer(const std::string &siteName,
                   wxWindow *parent,
                   wxWindowID id,
                   const wxPoint &pos,
                   const wxSize &size)
    : siteName_(siteName),
      wxSplitterWindow(parent,
                       id,
                       pos,
                       size,
                       wxSP_NOBORDER | wxSP_LIVE_UPDATE) {
    OssSiteNodePtr siteNode = ossSiteConfig()->get(siteName);

    localSite_.reset(new LocalSite);
    localSite_->WatchDirectoryCenter();
    ossSite_.reset(new OssSite(siteName));
    ossSite_->WatchDirectoryCenter();

    SetSashGravity(0.5);

    localListView_ = new LocalListView(this);
    localListView_->AttachSite(localSite_);
    localListView_->SetExplorer(this);

    ossListView_ = new OssListView(this);
    ossListView_->AttachSite(ossSite_);
    ossListView_->SetExplorer(this);

    localSite_->Attach(this);
    ossSite_->Attach(this);

    localListView_->GetListCtrl()->AddListener(this);
    ossListView_->GetListCtrl()->AddListener(this);

    localListView_->GetUriBox()->SetPeer(ossListView_->GetUriBox());
    ossListView_->GetUriBox()->SetPeer(localListView_->GetUriBox());

    localListView_->GetListCtrl()->SetPeer(ossListView_->GetListCtrl());
    ossListView_->GetListCtrl()->SetPeer(localListView_->GetListCtrl());

    SplitVertically(localListView_, ossListView_);

    localListView_->GetUriBox()->Bind(
            wxEVT_SET_FOCUS, &Explorer::ActivateLocalPanel, this);
    localListView_->GetListCtrl()->Bind(
            wxEVT_SET_FOCUS, &Explorer::ActivateLocalPanel, this);

    ossListView_->GetUriBox()->Bind(
            wxEVT_SET_FOCUS, &Explorer::ActivateOssPanel, this);
    ossListView_->GetListCtrl()->Bind(
            wxEVT_SET_FOCUS, &Explorer::ActivateOssPanel, this);

    if (siteNode) {
        localSite_->ChangeDir(siteNode->lastLocalPath);
        ossSite_->ChangeDir(siteNode->lastOssPath);
        localListView_->GetUriBox()->SetValue(siteNode->lastLocalPath);
        ossListView_->GetUriBox()->SetValue(siteNode->lastOssPath);
    }

    localListView_->GetListCtrl()->SetFocus();
}

Explorer::~Explorer() {
    localSite_->Detach(this);
    ossSite_->Detach(this);
}

void Explorer::willChangeDir(Site *site, const std::string &path) {
    if (inDiff_) {
        if (IsSubDir(site->comparePath_, path)) {
            std::string relativePath = RelativePath(site->comparePath_, path);
            peerSite_->ChangeDir(peerSite_->comparePath_ + relativePath);
        } else {
            QuitDiff();
        }
    }
}

void Explorer::Up() {
    if (!inDiff_) {
        activeSite_->Up();
    } else {
        std::string parentPath = activeSite_->GetParentPath();
        if (IsSubDir(activeSite_->comparePath_, parentPath)) {
            localSite_->Up();
            ossSite_->Up();
        } else {
            QuitDiff();
            activeSite_->Up();
        }
    }
}

void Explorer::Backward() {
    if (!activeSite_->CanBackward()) {
        return;
    }

    if (!inDiff_) {
        activeSite_->Backward();
    } else {
        std::string backwardPath = activeSite_->GetBackwardPath();
        if (IsSubDir(activeSite_->comparePath_, backwardPath)) {
            localSite_->Backward();
            ossSite_->Backward();
        } else {
            QuitDiff();
            activeSite_->Backward();
        }
    }
}

void Explorer::Forward() {
    if (!activeSite_->CanForward()) {
        return;
    }

    if (!inDiff_) {
        activeSite_->Forward();
    } else {
        std::string forwardPath = activeSite_->GetForwardPath();
        if (IsSubDir(activeSite_->comparePath_, forwardPath)) {
            localSite_->Forward();
            ossSite_->Forward();
        } else {
            QuitDiff();
            activeSite_->Forward();
        }
    }
}

void Explorer::RefreshFileItems() {
    activeSite_->Refresh();
}

void Explorer::OnTwinMode() {
    if (IsSplit()) {
        if (activeSite_ == localSite_.get()) {
            Unsplit(ossListView_);
        } else {
            Unsplit(localListView_);
        }
    } else {
        SplitVertically(localListView_, ossListView_);
    }
}

void Explorer::OnDiff() {
    if (!inDiff_) {
        if (ossSite_->GetCurrentDir()->tag == OssFilesTag) {
            inDiff_ = true;
            localSite_->comparePath_ = localSite_->GetCurrentPath();
            ossSite_->comparePath_ = ossSite_->GetCurrentPath();
            CompareDirectory(localSite_, ossSite_);
        }
    } else {
        QuitDiff();
    }
}

void Explorer::Copy() {
    int count = activeListCtrl_->GetSelectedItemCount();
    if (count > 0) {
        long item = -1;
        for (int i = 0; i < count; i++) {
            item = activeListCtrl_->GetNextItem(
                    item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            assert(item != -1);
            const auto &file = activeSite_->GetFiles()[item];
            std::string srcPath =
                    Site::Combine(activeSite_->GetCurrentPath(), file);
            std::string dstPath =
                    Site::Combine(peerSite_->GetCurrentPath(), file);
            taskList()->AddTask(TTCopy,
                                activeSite_->name(),
                                srcPath,
                                peerSite_->name(),
                                dstPath,
                                file->stat);
        }
    }
}

void Explorer::QuitDiff() {
    inDiff_ = false;
    if (localSite_->GetCurrentDir()->compareEnable) {
        localSite_->GetCurrentDir()->compareEnable = false;
        localSite_->NotifyChanged();
    }
    if (ossSite_->GetCurrentDir()->compareEnable) {
        ossSite_->GetCurrentDir()->compareEnable = false;
        ossSite_->NotifyChanged();
    }
    for (auto *l : listeners_) {
        l->ExplorerDiffQuit(this);
    }
}

void Explorer::ActivateLocalPanel(wxFocusEvent &event) {
    if (activeSite_ != localSite_.get()) {
        activeSite_ = localSite_.get();
        peerSite_ = ossSite_.get();
        activeListCtrl_ = localListView_->GetListCtrl();
        localListView_->ActiveBackground();
        ossListView_->InactiveBackground();
        Refresh();

        for (auto *l : listeners_) {
            l->ExplorerSiteSwitched(this, activeSite_);
        }
    }
    event.Skip();
}

void Explorer::ActivateOssPanel(wxFocusEvent &event) {
    if (activeSite_ != ossSite_.get()) {
        activeSite_ = ossSite_.get();
        peerSite_ = localSite_.get();
        activeListCtrl_ = ossListView_->GetListCtrl();
        ossListView_->ActiveBackground();
        localListView_->InactiveBackground();
        Refresh();

        for (auto *l : listeners_) {
            l->ExplorerSiteSwitched(this, activeSite_);
        }
    }
    event.Skip();
}

bool Explorer::IsPeerPath() const {
    if (!IsSubDir(localSite_->comparePath_, localSite_->GetCurrentPath())) {
        return false;
    }
    if (!IsSubDir(ossSite_->comparePath_, ossSite_->GetCurrentPath())) {
        return false;
    }

    return RelativePath(localSite_->comparePath_,
                        localSite_->GetCurrentPath()) ==
           RelativePath(ossSite_->comparePath_, ossSite_->GetCurrentPath());
}

void Explorer::SiteUpdated(Site *site) {
    OssSiteNodePtr ossSiteNode = ossSiteConfig()->get(siteName_);
    if (ossSiteNode) {
        if (site == localSite_.get()) {
            ossSiteNode->lastLocalPath = site->GetCurrentPath();
        } else if (site == ossSite_.get()) {
            ossSiteNode->lastOssPath = site->GetCurrentPath();
        }
    }
    if (inDiff_) {
        if (IsPeerPath()) {
            CompareDirectory(localSite_, ossSite_);
        }
    }
    for (auto *l : listeners_) {
        l->ExplorerSiteUpdated(this, site);
    }
}

void Explorer::SelectionUpdated(FileListCtrl *fileListCtrl) {
    for (auto *l : listeners_) {
        l->ExplorerSelectionUpdated(this, fileListCtrl);
    }
}

void Explorer::AddListener(ExplorerListener *l) {
    if (std::find(listeners_.begin(), listeners_.end(), l) ==
        listeners_.end()) {
        listeners_.push_back(l);
    }
}

void Explorer::RemoveListener(ExplorerListener *l) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), l),
                     listeners_.end());
}

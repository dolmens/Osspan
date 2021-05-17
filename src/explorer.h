#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/splitter.h>

#include "local_list_view.h"
#include "oss_list_view.h"
#include "panelex.h"
#include "task_list.h"

class Explorer;
class ExplorerListener {
public:
    virtual void ExplorerDiffQuit(Explorer *explorer) = 0;
    virtual void ExplorerSiteUpdated(Explorer *explorer, Site *site) = 0;
    virtual void ExplorerSiteSwitched(Explorer *explorer, Site *site) = 0;
    virtual void ExplorerSelectionUpdated(Explorer *explorer,
                                          FileListCtrl *fileListCtrl) = 0;
};

class Explorer : public wxSplitterWindow, SiteListener, FileListCtrlListener {
public:
    Explorer(const std::string &siteName,
             wxWindow *parent,
             wxWindowID id = wxID_ANY,
             const wxPoint &pos = wxDefaultPosition,
             const wxSize &size = wxDefaultSize);
    ~Explorer();

    void willChangeDir(Site *site, const std::string &path);

    void Up();
    void Backward();
    void Forward();
    void RefreshFileItems();

    void OnTwinMode();

    // 一次性比较，离开目录即失效
    void OnDiff();

    // 当前选中文件拷贝
    void Copy();

    const std::string &siteName() const { return siteName_; }

    const std::shared_ptr<LocalSite> &GetLocalSite() const {
        return localSite_;
    }

    const std::shared_ptr<OssSite> &GetOssSite() const { return ossSite_; }

    bool InDiffMode() const { return inDiff_; }

    bool InTwinMode() const { return IsSplit(); }

    FileListCtrl *GetActiveListCtrl() const { return activeListCtrl_; }

    Site *GetActiveSite() const { return activeSite_; }

    void AddListener(ExplorerListener *l);

    void RemoveListener(ExplorerListener *l);

private:
    void ActivateLocalPanel(wxFocusEvent &event);
    void ActivateOssPanel(wxFocusEvent &event);

    void SiteUpdated(Site *site) override;
    void SelectionUpdated(FileListCtrl *fileListCtrl) override;
    void QuitDiff();
    bool IsPeerPath() const;

    std::string siteName_;
    Site *activeSite_;
    Site *peerSite_;
    std::shared_ptr<LocalSite> localSite_;
    std::shared_ptr<OssSite> ossSite_;
    FileListCtrl *activeListCtrl_;
    LocalListView *localListView_;
    OssListView *ossListView_;

    bool inDiff_{false};

    std::shared_ptr<TaskList> taskList_;

    std::vector<ExplorerListener *> listeners_;
};

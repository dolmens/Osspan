#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/aui/aui.h>

#include "explorer.h"

class MultiTabExplorerListener : public ExplorerListener {
public:
    virtual void ExplorerSwitched(Explorer *explorer) = 0;
    virtual void TwinModeSwitched(Explorer *explorer, bool twinMode) = 0;
};

class MultiTabExplorer : public wxPanel, ExplorerListener {
public:
    MultiTabExplorer(wxWindow *parent,
                     wxWindowID winid = wxID_ANY,
                     const wxPoint &pos = wxDefaultPosition,
                     const wxSize &size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL | wxNO_BORDER,
                     const wxString &name = _T("MultiTabExplorer")

    );
    ~MultiTabExplorer();

    void Open(const std::string &ossSiteName);
    void Clear();

    void Up();
    void Backward();
    void Forward();
    void RefreshFileItems();

    void OnTwinMode();

    // 一次性比较，离开目录即失效
    void OnDiff();

    // 当前选中文件拷贝
    void Copy();

    void SetSelection(size_t index);
    size_t GetTabCount() const;

    std::vector<std::string> GetOpenedSites() const;
    int GetSelection() const;

    Explorer *GetActiveExplorer() const { return explorer_; }

    std::shared_ptr<LocalSite> GetCurrentLocalSite();
    std::shared_ptr<OssSite> GetCurrentOssSite();

    void AdvanceTab(bool forward);

    void AddListener(MultiTabExplorerListener *l);
    void RemoveListener(MultiTabExplorerListener *l);

private:
    void ExplorerSiteUpdated(Explorer *explorer, Site *site) override;
    void ExplorerSiteSwitched(Explorer *explorer, Site *site) override;
    void ExplorerSelectionUpdated(Explorer *explorer,
                                  FileListCtrl *fileListCtrl) override;
    void ExplorerDiffQuit(Explorer *explorer) override;

    void OnTabChanged(wxAuiNotebookEvent &event);
    void OnTabClosed(wxAuiNotebookEvent &event);
    void OnUnsplit(wxSplitterEvent &event);

    wxBoxSizer *mainSizer_;
    Explorer *explorer_{nullptr};
    wxAuiNotebook *tabs_{nullptr};

    std::vector<MultiTabExplorerListener *> listeners_;

    wxDECLARE_EVENT_TABLEex();
};

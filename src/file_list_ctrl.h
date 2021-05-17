#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include "defs.h"

#include "site.h"
#include "system_image_list.h"

class Explorer;
class UriBox;
class FileListCtrl;
class FileListCtrlListener {
public:
    virtual void SelectionUpdated(FileListCtrl *fileListCtrl) = 0;
};

class FileListCtrl : public wxListCtrl, public SiteHolder {
public:
    FileListCtrl(wxWindow *parent,
                 wxWindowID winid = wxID_ANY,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize);

    void SetUriBox(UriBox *uriBox) { uriBox_ = uriBox; }
    void SetPeer(FileListCtrl *peer) { peer_ = peer; }
    void SetExplorer(Explorer *explorer) { explorer_ = explorer; }
    std::string GetCurrentPath() const;
    const FilePtr &GetData(long item) const;

    int GetSelectedDirCount() const { return selectedDirCount_; }

    int GetSelectedFileCount() const { return selectedFileCount_; }

    int GetSelectedTotalSize() const { return selectedTotalSize_; }

    void AddListener(FileListCtrlListener *l);

    void RemoveListener(FileListCtrlListener *l);

protected:
    void InitCompareItemColors();
    void SiteUpdated(Site *site) override;

    void SetFileColumns();

    wxString OnGetItemText(long item, long column) const override;
    int OnGetItemImage(long item) const override;
    wxListItemAttr *OnGetItemAttr(long item) const override;

    std::string GetFileType(const std::string &file);

    void OnSize(wxSizeEvent &event);
    void OnLeftDown(wxMouseEvent &event);
    void OnFocusChanged(wxListEvent &event);
    void OnItemSelected(wxListEvent &event);
    void OnItemDeselected(wxListEvent &event);
    void OnKeyDown(wxKeyEvent &event);

    void OnProcessFocusChanged(wxCommandEvent& event);

    void ClearDropHighlight();
    void DisplayDropHighlight();

    void OnActivated(wxListEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnAdd(wxCommandEvent &event);
    void OnCopy(wxCommandEvent &event);
    void OnDelete(wxCommandEvent &event);

    void ShowContextMenu(const wxPoint &point, long item);
    void RemoveFiles();

    SystemImageList systemImageList_;
    std::map<std::string, std::string> fileTypeMap_;

    Explorer *explorer_{nullptr};
    UriBox *uriBox_{nullptr};
    FileListCtrl *peer_{nullptr};

    long lastFocusItem_;
    bool inSetState_{false};
    std::vector<bool> selections_;
    int selectedDirCount_;
    int selectedFileCount_;
    int selectedTotalSize_;

    void SelectionCleared();
    void SelectionUpdated(int min, int max);
    void NotifySelectionUpdated();

    long dropHighlightItem_{-1};
#ifndef __WXMSW__
    wxListItemAttr dropHighlightAttribute_;
#endif

protected:
    wxListItemAttr ItemAttrEqual_;    // default color
    wxListItemAttr ItemAttrNew_;      // green
    wxListItemAttr ItemAttrUpdated_;  // yellow
    wxListItemAttr ItemAttrOutdated_; // gray
    wxListItemAttr ItemAttrConflict_; // red

    std::vector<FileListCtrlListener *> listeners_;

    wxDECLARE_EVENT_TABLEex();
};

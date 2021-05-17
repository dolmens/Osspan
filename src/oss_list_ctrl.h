#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "defs.h"

#include "file_list_ctrl.h"
#include "oss_site.h"

class OssListCtrl : public FileListCtrl {
public:
    OssListCtrl(wxWindow *parent,
                wxWindowID winid = wxID_ANY,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize);
    ~OssListCtrl();

    OssSite *GetOssSite() const { return (OssSite *)(site_.get()); }

    int GetCurrentDirTag() const { return currentDirTag_; }

protected:
    friend class OssFileDropTarget;

    void SetBucketColumns();

    wxString OnGetItemText(long item, long column) const override;
    int OnGetItemImage(long item) const override;

    void OnContextMenu(wxContextMenuEvent &event);

    void OnBeginDrag(wxListEvent &event);

    void OnAdd(wxCommandEvent &event);
    void OnDelete(wxCommandEvent &event);

    void OnSize(wxSizeEvent &event);

    void ShowContextMenu(const wxPoint &point, long item);
    void CreateBucket();
    void RemoveBuckets();

private:
    void SiteUpdated(Site *site) override;

    int currentDirTag_;

    wxDECLARE_EVENT_TABLEex();
};

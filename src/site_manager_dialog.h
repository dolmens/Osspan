#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/treectrl.h>

#include "defs.h"
#include "dialogex.h"
#include "oss_site_config.h"

class SiteManagerDialog : public wxDialogEx {
public:
    SiteManagerDialog(wxWindow *parent, wxWindowID id);
    ~SiteManagerDialog() = default;

    OssSiteNode *GetSiteNode() const { return siteNode_; }
    void BuildSiteTree();

    void OnSelChanged(wxTreeEvent &event);
    void OnItemActivated(wxTreeEvent& event);
    void OnTreeItemMenu(wxTreeEvent &event);
    void OnNewItem(wxCommandEvent &event);
    void AddItem();
    void OnDeleteItem(wxCommandEvent &event);

    void OnOk(wxCommandEvent &event);
    void Ok();

    void EnableSiteEditors(bool enable = true);

private:
    OssSiteNode *siteNode_;
    wxTreeCtrl *siteTree_;
    wxTextCtrl *name_;
    wxComboBox *region_;
    wxTextCtrl *keyId_;
    wxTextCtrl *keySecret_;

    wxDECLARE_EVENT_TABLEex();
};

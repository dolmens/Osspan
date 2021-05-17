#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "panelex.h"
#include "uri_box.h"
class Explorer;

template <class Site, class UriBox, class FileListCtrl>
class FileListView : public wxPanel {
public:
    FileListView(wxWindow *parent) : wxPanel(parent) {
        auto *vbox = new wxBoxSizer(wxVERTICAL);
        SetSizer(vbox);
        uriPanel_ = new wxPanelEx(
                this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
        uriBox_ = new UriBox(uriPanel_);
        uriPanel_->SetWindow(uriBox_);
        vbox->Add(uriPanel_, 0, wxEXPAND);
        fileListPanel_ = new wxPanelEx(
                this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
        fileListPanel_->SetBorder(1);
        fileListCtrl_ = new FileListCtrl(fileListPanel_);
        fileListCtrl_->SetUriBox(uriBox_);
        fileListPanel_->SetWindow(fileListCtrl_);
        vbox->Add(fileListPanel_, 1, wxEXPAND);

        inactiveBackgroundColor_ = fileListPanel_->GetBackgroundColour();
        activeBackgroundColor_ = wxColor(169, 169, 169);
    }

    void AttachSite(const SitePtr &site) {
        uriBox_->AttachSite(site);
        fileListCtrl_->AttachSite(site);
    }

    void SetExplorer(Explorer *explorer) {
        explorer_ = explorer;
        uriBox_->SetExplorer(explorer_);
        fileListCtrl_->SetExplorer(explorer_);
    }

    void ActiveBackground() {
        fileListPanel_->SetBackgroundColour(activeBackgroundColor_);
    }

    void ActiveBackground(const wxColor &color) {
        fileListPanel_->SetBackgroundColour(color);
    }

    void InactiveBackground() {
        fileListPanel_->SetBackgroundColour(inactiveBackgroundColor_);
    }

    UriBox *GetUriBox() const { return uriBox_; }

    FileListCtrl *GetListCtrl() const { return fileListCtrl_; }

protected:
    Explorer *explorer_;
    wxPanelEx *uriPanel_;
    UriBox *uriBox_;
    wxPanelEx *fileListPanel_;
    FileListCtrl *fileListCtrl_;
    wxColor inactiveBackgroundColor_;
    wxColor activeBackgroundColor_;
};

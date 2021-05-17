#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "defs.h"
#include "dialogex.h"
#include "oss_list_view.h"

class OssBrowseDialog : public wxDialogEx {
public:
    OssBrowseDialog(wxWindow *parent, wxWindowID id);
    ~OssBrowseDialog() = default;

    void SetOssPath(const std::string &ossSiteName, const std::string &ossPath);

    wxString GetOssPath() const;

    void OnOk(wxCommandEvent &event);

    bool Validate() override;

private:
    OssListView *ossListView_;

    wxDECLARE_EVENT_TABLEex();
};

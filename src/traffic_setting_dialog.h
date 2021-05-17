#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/treectrl.h>

#include "defs.h"
#include "dialogex.h"

class TrafficSettingDialog : public wxDialogEx {
public:
    TrafficSettingDialog(wxWindow *parent, wxWindowID id);
    ~TrafficSettingDialog() = default;

    void OnOk(wxCommandEvent &event);

private:
    wxTextCtrl *downloadThreadsInput_;
    wxTextCtrl *uploadThreadsInput_;
    wxTextCtrl *downloadSpeedLimitInput_;
    wxCheckBox *downloadSpeedEnableBox_;
    wxTextCtrl *uploadSpeedLimitInput_;
    wxCheckBox *uploadSpeedEnableBox_;

    wxDECLARE_EVENT_TABLEex();
};

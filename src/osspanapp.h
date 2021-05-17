#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "mainfrm.h"

class App : public wxApp {
public:
    virtual bool OnInit();

    MainFrame *mainFrame() const { return mainFrame_; }

    wxString GetUserDataDir() { return userDataDir_; }

private:
    std::unique_ptr<wxLocale> locale_;
    wxString userDataDir_;
    MainFrame *mainFrame_;
};

wxDECLARE_APP(App);

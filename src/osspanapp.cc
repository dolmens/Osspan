#include "osspanapp.h"

#include <wx/sysopt.h>
#include <wx/stdpaths.h>

#include <filesystem>

bool App::OnInit() {
    // Only send idle events to specified windows
    wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);

    wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);

    wxSystemOptions::SetOption(_T("window-default-variant"), _T(""));

    locale_.reset(new wxLocale(wxLANGUAGE_DEFAULT));
    if (!locale_->AddCatalog(_T("Osspan"))) {
        wxLogError(
                _T("Couldn't find/load the 'Osspan' catalog for locale '%s'."),
                _T(""));
    }
    locale_->AddCatalog(_T("wxstd"));

    userDataDir_ = wxStandardPaths::Get().GetUserDataDir();
    if (!std::filesystem::exists(userDataDir_.ToStdString())) {
        std::error_code ec;
        std::filesystem::create_directory(userDataDir_.ToStdString(), ec);
    }

    oss::InitializeSdk();
    mainFrame_ = new MainFrame();
    mainFrame_->Show();
    return true;
}

wxIMPLEMENT_APP(App);

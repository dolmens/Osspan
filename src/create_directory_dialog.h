#pragma once

#include "defs.h"
#include "dialogex.h"
#include "site.h"

class CreateDirectoryDialog final : public wxDialogEx {
public:
    CreateDirectoryDialog(Site *site, wxWindow *parent);

    void OnOk(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);

private:
    Site *site_;
    wxTextCtrl *textCtrl_{};
    wxDECLARE_EVENT_TABLEex();
};

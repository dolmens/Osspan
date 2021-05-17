#pragma once

#include "dialogex.h"

class InputDialog final : public wxDialogEx {
public:
    InputDialog(wxWindow *parent,
            wxString const &title,
            wxString const &text,
            int max_len = -1,
            bool password = false);

    void AllowEmpty(bool allowEmpty);

    void SetValue(wxString const &value);
    wxString GetValue() const;

    bool SelectText(int start, int end);

protected:
    bool allowEmpty_{};
    wxTextCtrl *textCtrl_{};
};

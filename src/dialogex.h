#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class DialogLayout final {
public:
    int gap;
    int border;
    int indent;

    int dlgUnits(int num) const;

    static wxSizerFlags const grow;
    static wxSizerFlags const halign;
    static wxSizerFlags const valign;
    static wxSizerFlags const valigng;
    static wxSizerFlags const ralign;

    wxFlexGridSizer *createMain(wxWindow *parent,
            int cols = 1,
            int rows = 0) const;
    wxFlexGridSizer *createFlex(int cols = 1, int rows = 0) const;
    wxGridSizer *createGrid(int cols = 1, int rows = 0) const;
    wxStdDialogButtonSizer *createButtonSizer(wxWindow *parent,
            wxSizer *sizer,
            bool hline) const;

    std::tuple<wxStaticBox *, wxFlexGridSizer *> createStatBox(wxSizer *parent,
            wxString const &title,
            int cols = 1,
            int rows = 0) const;

    DialogLayout(wxDialog *dialog);

private:
    wxDialog *dialog_;
};

class wxDialogEx : public wxDialog {
public:
    using wxDialog::wxDialog;

    const DialogLayout &layout();

protected:
    std::unique_ptr<DialogLayout> layout_;
};

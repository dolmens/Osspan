#include "dialogex.h"
#include <wx/statline.h>

wxSizerFlags const DialogLayout::grow(wxSizerFlags().Expand());
wxSizerFlags const
        DialogLayout::valign(wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
wxSizerFlags const
        DialogLayout::halign(wxSizerFlags().Align(wxALIGN_CENTER_HORIZONTAL));
wxSizerFlags const DialogLayout::valigng(
        wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Expand());
wxSizerFlags const DialogLayout::ralign(wxSizerFlags().Align(wxALIGN_RIGHT));

DialogLayout::DialogLayout(wxDialog *dialog) {
    dialog_ = dialog;
    gap = dlgUnits(3);
    border = dlgUnits(3);
    indent = dlgUnits(10);
}

int DialogLayout::dlgUnits(int num) const {
    return wxDLG_UNIT(dialog_, wxPoint(0, num)).y;
}

wxFlexGridSizer *
DialogLayout::createMain(wxWindow *parent, int cols, int rows) const {
    auto *outer = new wxBoxSizer(wxVERTICAL);
    parent->SetSizer(outer);

    auto main = createFlex(cols, rows);
    outer->Add(main, 1, wxEXPAND | wxALL, border);

    return main;
}

wxFlexGridSizer *DialogLayout::createFlex(int cols, int rows) const {
    return new wxFlexGridSizer(rows, cols, gap, gap);
}

wxGridSizer *DialogLayout::createGrid(int cols, int rows) const {
    return new wxGridSizer(rows, cols, gap, gap);
}

wxStdDialogButtonSizer *DialogLayout::createButtonSizer(wxWindow *parent,
        wxSizer *sizer,
        bool hline) const {
    if (hline) {
        sizer->Add(new wxStaticLine(parent), grow);
    }

    auto btns = new wxStdDialogButtonSizer;
    sizer->Add(btns, grow);

    return btns;
}

const DialogLayout &wxDialogEx::layout() {
    if (!layout_) {
        layout_ = std::make_unique<DialogLayout>(this);
    }
    return *layout_;
}

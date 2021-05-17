#include "panelex.h"

wxPanelEx::wxPanelEx(wxWindow *parent,
                     wxWindowID winid,
                     const wxPoint &pos,
                     const wxSize &size,
                     long style)
    : wxPanel(parent, winid, pos, size, style) {
    sizer_ = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer_);
}

void wxPanelEx::SetBorder(int border) {
    border_ = border;
}

void wxPanelEx::SetWindow(wxWindow *window) {
    sizer_->Add(window, 1, wxEXPAND | wxALL, border_);
}

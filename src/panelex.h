#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/**
 * wxPanel with default style wxSUNKEN_BORDER and a wxBoxSizer
 */
class wxPanelEx : public wxPanel {
public:
    wxPanelEx(wxWindow *parent,
              wxWindowID winid = wxID_ANY,
              const wxPoint &pos = wxDefaultPosition,
              const wxSize &size = wxDefaultSize,
              long style = wxSUNKEN_BORDER);

    void SetBorder(int border);
    void SetWindow(wxWindow *window);

protected:
    wxBoxSizer *sizer_;
    int border_{0};
};

#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class Led final : public wxWindow {
public:
    Led(wxWindow *parent, unsigned int index);
    void Set(bool on = true);
    void Unset();
    bool IsOn() const;

protected:
    void OnPaint(wxPaintEvent &event);

    int state_;
    wxBitmap leds_[2];
    bool loaded_{false};

    DECLARE_EVENT_TABLE()
};

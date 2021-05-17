#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "led.h"

class StatusBar : public wxStatusBar {
public:
    StatusBar(wxWindow *parent);
    void UpdateActivityLed();
    void Light();

    void OnTimer(wxTimerEvent& event);
    void OnSize(wxSizeEvent &event);


    void OnLeftMouseUp(wxMouseEvent& event);

private:
    Led *leds_[2];
    wxTimer timer_;
    int timerIdle_;

    DECLARE_EVENT_TABLE()
};

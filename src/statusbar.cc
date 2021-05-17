#include "statusbar.h"

#include "traffic.h"
#include "traffic_setting_dialog.h"

// clang-format off
BEGIN_EVENT_TABLE(StatusBar, wxStatusBar)
EVT_TIMER(wxID_ANY, StatusBar::OnTimer)
EVT_SIZE(StatusBar::OnSize)
EVT_LEFT_UP(StatusBar::OnLeftMouseUp)
END_EVENT_TABLE();
// clang-format on

StatusBar::StatusBar(wxWindow *parent)
    : wxStatusBar(parent, wxID_ANY, wxSTB_DEFAULT_STYLE) {
    int widths[] = {-1, 16, 16, 8};
    SetFieldsCount(4, widths);
    leds_[0] = new Led(this, 1);
    leds_[1] = new Led(this, 0);

    leds_[0]->Bind(wxEVT_LEFT_UP, &StatusBar::OnLeftMouseUp, this);
    leds_[1]->Bind(wxEVT_LEFT_UP, &StatusBar::OnLeftMouseUp, this);

    timer_.SetOwner(this);
}

void StatusBar::UpdateActivityLed() {
    if (timer_.IsRunning()) {
        return;
    }
    Light();
    timerIdle_ = 0;
    timer_.Start(100);
}

void StatusBar::Light() {
    leds_[(int)Direction::Send]->Set(Traffic::IsOn(Direction::Send));
    leds_[(int)Direction::Recv]->Set(Traffic::IsOn(Direction::Recv));
}

void StatusBar::OnTimer(wxTimerEvent &event) {
    if (!timer_.IsRunning()) {
        return;
    }
    if (event.GetId() != timer_.GetId()) {
        return;
    }
    Light();
    if (!leds_[0]->IsOn() && !leds_[1]->IsOn()) {
        timerIdle_++;
        if (timerIdle_ == 10) {
            timer_.Stop();
        }
    } else {
        timerIdle_ = 0;
    }
}

void StatusBar::OnSize(wxSizeEvent &event) {
    wxRect rect;
    GetFieldRect(0, rect);

    GetFieldRect(1, rect);
    wxSize size = leds_[0]->GetSize();
    leds_[0]->Move(rect.x + (rect.width - size.x) / 2,
                   rect.y + (rect.height - size.y) / 2);

    GetFieldRect(2, rect);
    leds_[1]->Move(rect.x + (rect.width - size.x) / 2,
                   rect.y + (rect.height - size.y) / 2);

    event.Skip();
}

void StatusBar::OnLeftMouseUp(wxMouseEvent &event) {
    TrafficSettingDialog dlg(this, wxID_ANY);
    dlg.ShowModal();
}

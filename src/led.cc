#include "led.h"
#include "theme_provider.h"

// clang-format off
wxBEGIN_EVENT_TABLE(Led, wxWindow)
EVT_PAINT(Led::OnPaint)
wxEND_EVENT_TABLE();
// clang-format on

enum { LED_ON = 0, LED_OFF = 1 };

Led::Led(wxWindow *parent, unsigned int index) {
    const wxSize &size = themeProvider()->GetIconSize(iconSizeTiny);
    Create(parent, wxID_ANY, wxDefaultPosition, size);
    wxBitmap bmp =
            themeProvider()->CreateBitmap("ART_LEDS", wxART_OTHER, size * 2);
    if (bmp.IsOk()) {
        leds_[LED_ON] =
                bmp.GetSubBitmap(wxRect(0, index * size.y, size.x, size.y));
        leds_[LED_OFF] = bmp.GetSubBitmap(
                wxRect(size.x, index * size.y, size.x, size.y));
        loaded_ = true;
        state_ = LED_OFF;
    }
}

void Led::Set(bool on) {
    if (on) {
        if (state_ != LED_ON) {
            state_ = LED_ON;
            Refresh();
        }
    } else {
        Unset();
    }
}

void Led::Unset() {
    if (state_ != LED_OFF) {
        state_ = LED_OFF;
        Refresh();
    }
}

bool Led::IsOn() const {
    return state_ == LED_ON;
}

void Led::OnPaint(wxPaintEvent &event) {
    wxPaintDC dc(this);

    if (!loaded_) {
        return;
    }

    dc.DrawBitmap(leds_[state_], 0, 0, true);
}

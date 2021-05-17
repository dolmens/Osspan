#include "window_state_manager.h"
#include "options.h"
#include "utils.h"

#include <sstream>
#include <wx/display.h>

WindowStateManager::WindowStateManager(wxTopLevelWindow *window) {
    window_ = window;
}

WindowStateManager::~WindowStateManager() {
}

wxRect WindowStateManager::GetScreenDimension() {
    wxRect screenSize{0, 0, 0, 0};

    for (auto i = 0; i < wxDisplay::GetCount(); i++) {
        wxDisplay display(i);
        wxRect rect = display.GetGeometry();
        screenSize.Union(rect);
    }
    if (screenSize.GetWidth() <= 0 || screenSize.GetHeight() <= 0) {
        screenSize = wxRect(0, 0, 1000000000, 1000000000);
    }

    return screenSize;
}

void WindowStateManager::Restore() {
    const std::vector<int> & positions =
            options().get_int_vec(OPTION_MAINWINDOW_POSITION);
    if (positions.size() < 4) {
        window_->CenterOnScreen();
        return;
    }

    wxPoint pos(positions[0], positions[1]);
    wxSize size(positions[2], positions[3]);

    if (window_->GetSizer()) {
        wxSize minSize = window_->GetSizer()->GetMinSize();
        if (minSize.IsFullySpecified()) {
            size.x = std::max(size.x, minSize.x);
            size.y = std::max(size.y, minSize.y);
        }
    }

    window_->Move(pos.x, pos.y);

    if (size.IsFullySpecified()) {
        window_->SetClientSize(size);
    }
}

void WindowStateManager::Remember() {
    wxPoint pos = window_->GetPosition();
    wxSize size = window_->GetClientSize();
    options().set(
            OPTION_MAINWINDOW_POSITION,
            std::vector<int>{pos.x, pos.y, size.GetWidth(), size.GetHeight()});
}

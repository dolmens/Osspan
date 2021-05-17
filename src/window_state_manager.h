#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class WindowStateManager final : public wxEvtHandler {
public:
    explicit WindowStateManager(wxTopLevelWindow *window);
    virtual ~WindowStateManager();

    static wxRect GetScreenDimension();

    void Restore();
    void Remember();

private:
    wxTopLevelWindow *window_;
};

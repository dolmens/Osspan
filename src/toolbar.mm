#include <wx/wx.h>

#include <AppKit/NSWindow.h>

void fix_macos_window_style(wxFrame *frame) {
        if (__builtin_available(macos 11.0, *)) {
                WXWindow w = frame->MacGetTopLevelWindowRef();
                //[w setToolbarStyle:NSWindowToolbarStyleExpanded];
                [w setTabbingMode:NSWindowTabbingModeDisallowed];
        }
}

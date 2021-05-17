#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>
#include "defs.h"
#include "schedule_list.h"

class ScheduleListCtrl : public wxListCtrl {
public:
    ScheduleListCtrl(wxWindow *parent,
                     wxWindowID winid = wxID_ANY,
                     const wxPoint &pos = wxDefaultPosition,
                     const wxSize &size = wxDefaultSize);
    ~ScheduleListCtrl();

    void RemoveSelectedSchedules();

protected:
    wxString OnGetItemText(long item, long column) const override;

    void OnItemSelected(wxListEvent &event);
    void OnContextMenu(wxContextMenuEvent &event);
    void OnDelete(wxCommandEvent &event);

    wxDECLARE_EVENT_TABLEex();
};

#include "schedule_list_ctrl.h"
#include "schedule_list.h"
#include "string_format.h"

// clang-format off
wxBEGIN_EVENT_TABLE(ScheduleListCtrl, wxListCtrl)
// EVT_LIST_ITEM_SELECTED(wxID_ANY, ScheduleListCtrl::OnItemSelected)
EVT_CONTEXT_MENU(ScheduleListCtrl::OnContextMenu)
EVT_MENU(wxID_DELETE, ScheduleListCtrl::OnDelete)
wxEND_EVENT_TABLE();
// clang-format on

ScheduleListCtrl::ScheduleListCtrl(wxWindow *parent,
                                   wxWindowID winid,
                                   const wxPoint &pos,
                                   const wxSize &size)
    : wxListCtrl(parent, winid, pos, size, wxLC_REPORT | wxLC_VIRTUAL) {

    const int widths[] = {80, 80, 60, 80, 256, 80, 256};
    AppendColumn(_("name"), wxLIST_FORMAT_LEFT, widths[0]);
    AppendColumn(_("create time"), wxLIST_FORMAT_LEFT, widths[1]);
    AppendColumn(_("type"), wxLIST_FORMAT_LEFT, widths[2]);
    AppendColumn(_("last start time"), wxLIST_FORMAT_LEFT, widths[3]);
    AppendColumn(_("srcPath"), wxLIST_FORMAT_LEFT, widths[4]);
    AppendColumn(_("dstSite"), wxLIST_FORMAT_LEFT, widths[5]);
    AppendColumn(_("dstPath"), wxLIST_FORMAT_LEFT, widths[6]);

    const SchedulePtrVec &schedules = scheduleList()->schedules();
    SetItemCount(schedules.size());
}

ScheduleListCtrl::~ScheduleListCtrl() {
}

void ScheduleListCtrl::RemoveSelectedSchedules() {
    long item = -1;
    while ((item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) !=
           -1) {
        scheduleList()->Remove(item);
        DeleteItem(item);
        item--;
    }
}

wxString ScheduleListCtrl::OnGetItemText(long item, long column) const {
    // return std::to_string(item) + ":" + std::to_string(column);
    const SchedulePtr &schedule = scheduleList()->schedules()[item];
    switch (column) {
    case 0:
        return schedule->name;
    case 1:
        return opstrftimew(schedule->lastStartTime);
    case 2:
        return ScheduleRoutineToWString(schedule->routine);
    case 3:
        return opstrftimew(schedule->lastStartTime);
    case 4:
        return schedule->srcPath;
    case 5:
        return schedule->dstSite;
    case 6:
        return schedule->dstPath;
    default:
        return _T("");
    }
}

void ScheduleListCtrl::OnItemSelected(wxListEvent &event) {

}

void ScheduleListCtrl::OnContextMenu(wxContextMenuEvent &event) {
    wxPoint point = event.GetPosition();
    // If from keyboard
    if (point.x == -1 && point.y == -1) {
        wxSize size = GetSize();
        point.x = size.x / 2;
        point.y = size.y / 2;
    } else {
        point = ScreenToClient(point);
    }

    int flags;
    long item = HitTest(point, flags);

    wxMenu menu;
    menu.Append(wxID_NEW, _("New"));
    menu.Append(wxID_DELETE, _("Delete"))->Enable(GetSelectedItemCount() > 0);

    PopupMenu(&menu, point.x, point.y);
}

void ScheduleListCtrl::OnDelete(wxCommandEvent &event) {
    RemoveSelectedSchedules();
}

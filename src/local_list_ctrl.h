#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "file_list_ctrl.h"
#include "local_site.h"

#include <string>
#include <vector>

class LocalListCtrl : public FileListCtrl {
public:
    LocalListCtrl(wxWindow *parent,
                  wxWindowID winid = wxID_ANY,
                  const wxPoint &pos = wxDefaultPosition,
                  const wxSize &size = wxDefaultSize);
    ~LocalListCtrl();

    LocalSite *GetLocalSite() const { return (LocalSite *)site_.get(); }

protected:
    friend class LocalFileDropTarget;
    void OnBeginDrag(wxListEvent &event);

private:
    wxDECLARE_EVENT_TABLEex();
};

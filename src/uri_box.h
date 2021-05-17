#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "defs.h"
#include "site.h"

class Explorer;
class UriBox : public wxComboBox, public SiteHolder {
public:
    UriBox(wxWindow *parent);

    void SiteUpdated(Site *site) override;

    void SetPeer(UriBox *peer) { peer_ = peer; }

    void SetExplorer(Explorer *explorer) { explorer_ = explorer; }

protected:
    void OnKeyDown(wxKeyEvent &event);
    void Go(wxCommandEvent &evt);

    virtual std::string Format() = 0;

    UriBox *peer_{nullptr};

    Explorer *explorer_{nullptr};

    wxDECLARE_EVENT_TABLEex();
};

#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include "defs.h"
#include "dialogex.h"
#include "oss_site.h"
#include "site.h"

class RemoveObjectsDialogBase : public wxDialogEx {
public:
    using wxDialogEx::wxDialogEx;
    virtual void OnOk(wxCommandEvent &event) = 0;

protected:
    bool processing_{false};
    std::atomic_bool cancel_{false};
    std::atomic_int pending_{0};
    int total_;
};

class RemoveObjectsListCtrl : public wxListCtrl {
public:
    RemoveObjectsListCtrl(const std::vector<std::string> &items,
                          RemoveObjectsDialogBase *parent);
    void Update(long item, bool status);

protected:
    wxString OnGetItemText(long item, long column) const override;
    int OnGetItemImage(long item) const override;

private:
    void OnKeyDown(wxListEvent &event);

    const std::vector<std::string> &items_;
    std::vector<int> status_;
    RemoveObjectsDialogBase *parent_;

    wxDECLARE_EVENT_TABLEex();
};

class RemoveObjectsDialog : public RemoveObjectsDialogBase {
public:
    RemoveObjectsDialog(Site *site,
                        const std::vector<std::string> &items,
                        wxWindow *parent);
    ~RemoveObjectsDialog() = default;

    void OnOk(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);

protected:
    void Update(long item, bool status);

private:
    Site *site_;
    const std::vector<std::string> &items_;
    RemoveObjectsListCtrl *listCtrl_;
    bool allOk_;
    wxDECLARE_EVENT_TABLEex();
};

class RemoveBucketsDialog : public RemoveObjectsDialogBase {
public:
    RemoveBucketsDialog(OssSite *site,
                        const std::vector<std::string> &items,
                        wxWindow *parent);
    ~RemoveBucketsDialog() = default;

    void OnOk(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);

protected:
    void Update(long item, bool status);

private:
    OssSite *site_;
    const std::vector<std::string> &items_;
    RemoveObjectsListCtrl *listCtrl_;
    bool allOk_;
    wxDECLARE_EVENT_TABLEex();
};

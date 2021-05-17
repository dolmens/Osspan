#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/dnd.h>

#include "task_list.h"

#include <string>
#include <vector>

class DragDropManager final {
public:
    static DragDropManager *Get() {
        static DragDropManager *instance = new DragDropManager;
        return instance;
    }

    std::string sourceDirectory;

protected:
    DragDropManager() = default;
    ~DragDropManager() = default;
};

class FileDataObject : public wxDataObjectSimple {
public:
    FileDataObject();

    ~FileDataObject();

    size_t GetDataSize() const override;

    bool GetDataHere(void *buf) const override;

    bool SetData(size_t len, const void *buf) override;

    void SetSite(const std::string &site) { site_ = site; }
    void AddFile(const std::string &file, const FileStat &stat) {
        files_.emplace_back(file, stat);
    }

    const std::string &GetSite() const { return site_; }
    const std::vector<std::pair<std::string, FileStat>> &GetFiles() const {
        return files_;
    }

protected:
    std::string site_;
    std::vector<std::pair<std::string, FileStat>> files_;
};

template <class FileListCtrl> class FileDropTarget : public wxDropTarget {
public:
    FileDropTarget(FileListCtrl *listCtrl = nullptr) : fileListCtrl_(listCtrl) {
        dataObject_ = new wxDataObjectComposite;
        fileDataObject_ = new FileDataObject;
        dataObject_->Add(fileDataObject_);
        sysFileDataObject_ = new wxFileDataObject;
        dataObject_->Add(sysFileDataObject_);
        SetDataObject(dataObject_);
    }

    bool OnDrop(wxCoord x, wxCoord y) override { return true; }

    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) override;

    wxDragResult FixupDragResult(wxDragResult res) {
        if (res == wxDragLink) {
            res = wxGetKeyState(WXK_ALT) ? wxDragCopy : wxDragMove;
            return res;
        }

        return res;
    }

protected:
    FileListCtrl *fileListCtrl_;
    wxDataObjectComposite *dataObject_;
    FileDataObject *fileDataObject_;
    wxFileDataObject *sysFileDataObject_;
};

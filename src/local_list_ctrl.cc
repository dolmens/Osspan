#include "local_list_ctrl.h"
#include "system_image_list.h"

#include "dndobjects.h"
#include "task_list.h"

class LocalFileDropTarget final : public FileDropTarget<LocalListCtrl> {
public:
    using FileDropTarget<LocalListCtrl>::FileDropTarget;
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override {
        bool sameDirectory = DragDropManager::Get()->sourceDirectory ==
                             fileListCtrl_->GetCurrentPath();
        int flags;
        int hit = fileListCtrl_->HitTest(wxPoint(x, y), flags);
        if (hit == wxNOT_FOUND) {
            fileListCtrl_->ClearDropHighlight();
            return sameDirectory ? wxDragNone : wxDragCopy;
        }

        if (fileListCtrl_->GetItemState(hit, wxLIST_STATE_SELECTED)) {
            if (sameDirectory) {
                fileListCtrl_->ClearDropHighlight();
                return wxDragNone;
            }
        }

        const FilePtr &file = fileListCtrl_->GetData(hit);
        if (file->type != FTDirectory) {
            fileListCtrl_->ClearDropHighlight();
            return sameDirectory ? wxDragNone : wxDragCopy;
        }

        if (fileListCtrl_->dropHighlightItem_ != hit) {
            fileListCtrl_->ClearDropHighlight();
            fileListCtrl_->dropHighlightItem_ = hit;
            fileListCtrl_->DisplayDropHighlight();
        }

        return wxDragCopy;
    }
};

// clang-format off
wxBEGIN_EVENT_TABLE(LocalListCtrl, FileListCtrl)
EVT_LIST_BEGIN_DRAG(wxID_ANY, LocalListCtrl::OnBeginDrag)
wxEND_EVENT_TABLE();
// clang-format on

LocalListCtrl::LocalListCtrl(wxWindow *parent,
                             wxWindowID winid,
                             const wxPoint &pos,
                             const wxSize &size)
    : FileListCtrl(parent, winid, pos, size) {
    SetFileColumns();
    SetDropTarget(new LocalFileDropTarget(this));
}

LocalListCtrl::~LocalListCtrl() {
}

void LocalListCtrl::OnBeginDrag(wxListEvent &event) {
#ifdef __WXMAC__
    // Don't use wxFileDataObject on Mac, crashes on Mojave, wx bug #18232
    FileDataObject dataObject;
#else
    wxFileDataObject dataObject;
#endif

    bool added = false;
    for (int item = -1;;) {
        item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1) {
            break;
        }
        const auto &file = site_->GetFiles()[item];
        if (file->type == FTDirectory && file->name == "..") {
            continue;
        }
        std::string name = Site::Combine(site_->GetCurrentPath(), file);
        dataObject.AddFile(name, file->stat);
        added = true;
    }

    if (!added) {
        return;
    }

    DragDropManager::Get()->sourceDirectory = GetCurrentPath();

    wxDropSource source(this);
    source.SetData(dataObject);
    wxDragResult result = source.DoDragDrop(wxDrag_AllowMove);
}

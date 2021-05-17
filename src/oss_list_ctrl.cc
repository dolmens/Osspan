#include "oss_list_ctrl.h"
#include "create_bucket_dialog.h"
#include "remove_objects_dialog.h"

#include "string_format.h"
#include "dndobjects.h"
#include "task_list.h"

class OssFileDropTarget final : public FileDropTarget<OssListCtrl> {
public:
    using FileDropTarget<OssListCtrl>::FileDropTarget;
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override {
        int currentDirTag = fileListCtrl_->GetCurrentDirTag();
        if (currentDirTag == OssBucketsTag) {
            int flags;
            int hit = fileListCtrl_->HitTest(wxPoint(x, y), flags);
            if (hit == wxNOT_FOUND) {
                fileListCtrl_->ClearDropHighlight();
                return wxDragNone;
            }
            if (fileListCtrl_->dropHighlightItem_ != hit) {
                fileListCtrl_->ClearDropHighlight();
                fileListCtrl_->dropHighlightItem_ = hit;
                fileListCtrl_->DisplayDropHighlight();
            }
            return wxDragCopy;
        } else if (currentDirTag == OssFilesTag) {
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

        return wxDragNone;
    }
};

// clang-format off
wxBEGIN_EVENT_TABLE(OssListCtrl, FileListCtrl)
EVT_CONTEXT_MENU(OssListCtrl::OnContextMenu)
EVT_LIST_BEGIN_DRAG(wxID_ANY, OssListCtrl::OnBeginDrag)
EVT_MENU(wxID_ADD, OssListCtrl::OnAdd)
EVT_MENU(wxID_DELETE, OssListCtrl::OnDelete)
EVT_SIZE(OssListCtrl::OnSize)
wxEND_EVENT_TABLE();
// clang-format on

OssListCtrl::OssListCtrl(wxWindow *parent,
                         wxWindowID winid,
                         const wxPoint &pos,
                         const wxSize &size)
    : FileListCtrl(parent, winid, pos, size) {
    SetDropTarget(new OssFileDropTarget(this));
}

OssListCtrl::~OssListCtrl() {
}

void OssListCtrl::SetBucketColumns() {
    ClearAll();
    const int widths[4] = {200, 100, 120};
    AppendColumn(_("Bucket"), wxLIST_FORMAT_LEFT, widths[0]);
    AppendColumn(_("Location"), wxLIST_FORMAT_LEFT, widths[1]);
    AppendColumn(_("Create Time"), wxLIST_FORMAT_LEFT, widths[2]);
}

wxString OssListCtrl::OnGetItemText(long item, long column) const {
    if (currentDirTag_ == OssBucketsTag) {
        const auto &file = site_->GetFiles()[item];
        if (column == 0) {
            return file->name;
        }
        if (column == 1) {
            return file->location;
        }
        if (column == 2) {
            return opstrftimew(file->stat.lastModifiedTime);
        }
        assert(0);
    }

    return FileListCtrl::OnGetItemText(item, column);
}

int OssListCtrl::OnGetItemImage(long item) const {
    if (currentDirTag_ == OssBucketsTag) {
        return 6;
    }
    return FileListCtrl::OnGetItemImage(item);
}

void OssListCtrl::OnContextMenu(wxContextMenuEvent &event) {
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
    ShowContextMenu(point, item);
}

void OssListCtrl::OnBeginDrag(wxListEvent &event) {
    if (currentDirTag_ != OssFilesTag) {
        return;
    }
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

    dataObject.SetSite(site_->name());

    DragDropManager::Get()->sourceDirectory = GetCurrentPath();

    wxDropSource source(this);
    source.SetData(dataObject);
    wxDragResult result = source.DoDragDrop(wxDrag_AllowMove);
}

void OssListCtrl::OnAdd(wxCommandEvent &event) {
    if (currentDirTag_ == OssBucketsTag) {
        CreateBucket();
    } else {
        event.Skip();
    }
}

void OssListCtrl::OnDelete(wxCommandEvent &event) {
    if (currentDirTag_ == OssBucketsTag) {
        RemoveBuckets();
    } else {
        event.Skip();
    }
}

void OssListCtrl::OnSize(wxSizeEvent &event) {
    if (currentDirTag_ == OssBucketsTag) {
        event.Skip();
    } else {
        FileListCtrl::OnSize(event);
    }
}

void OssListCtrl::ShowContextMenu(const wxPoint &point, long item) {
    if (currentDirTag_ == OssBucketsTag) {
        wxMenu menu;
        menu.Append(wxID_DELETE, _("Delete"));
        menu.Append(wxID_ADD, _("Create Bucket"));

	int index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (index == -1) {
            menu.Enable(wxID_DELETE, false);
        }
        PopupMenu(&menu, point.x, point.y);
    } else {
        FileListCtrl::ShowContextMenu(point, item);
    }
}

void OssListCtrl::CreateBucket() {
    CreateBucketDialog dlg(GetOssSite(), this);
    dlg.ShowModal();
}

void OssListCtrl::RemoveBuckets() {
    std::vector<std::string> items;
    int count = GetSelectedItemCount();
    if (count > 0) {
        long item = -1;
        for (int i = 0; i < count; i++) {
            item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            assert(item != -1);
            const auto &file = site_->GetFiles()[item];
            items.push_back(file->name);
        }
    }
    RemoveBucketsDialog dlg(GetOssSite(), items, this);
    dlg.ShowModal();
}

void OssListCtrl::SiteUpdated(Site *site) {
    int currentDirTag = site_->GetCurrentDir()->tag;
    if (currentDirTag_ != currentDirTag) {
        currentDirTag_ = currentDirTag;
        if (currentDirTag_ == OssBucketsTag) {
            SetBucketColumns();
        } else if (currentDirTag_ == OssFilesTag) {
            SetFileColumns();
        } else {
            assert(0);
            ClearAll();
        }
    }
    FileListCtrl::SiteUpdated(site);
}

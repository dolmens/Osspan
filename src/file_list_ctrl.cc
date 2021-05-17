#include "file_list_ctrl.h"
#include "create_directory_dialog.h"
#include "explorer.h"
#include "remove_objects_dialog.h"
#include "string_format.h"
#include "theme_provider.h"
#include "uri_box.h"

#include <wx/mimetype.h>

#include <memory>
#include <sstream>

namespace {
enum {
    opID_COPY = wxID_HIGHEST + 1,
};
}

wxDECLARE_EVENT(op_EVT_FILELIST_FOCUSCHANGE, wxCommandEvent);
wxDEFINE_EVENT(op_EVT_FILELIST_FOCUSCHANGE, wxCommandEvent);

// clang-format off
wxBEGIN_EVENT_TABLE(FileListCtrl, wxListCtrl)
EVT_LIST_ITEM_ACTIVATED(wxID_ANY, FileListCtrl::OnActivated)
EVT_CONTEXT_MENU(FileListCtrl::OnContextMenu)
EVT_MENU(wxID_ADD, FileListCtrl::OnAdd)
EVT_MENU(opID_COPY, FileListCtrl::OnCopy)
EVT_MENU(wxID_DELETE, FileListCtrl::OnDelete)
EVT_SIZE(FileListCtrl::OnSize)
EVT_LEFT_DOWN(FileListCtrl::OnLeftDown)
EVT_LIST_ITEM_FOCUSED(wxID_ANY, FileListCtrl::OnFocusChanged)
EVT_COMMAND(wxID_ANY, op_EVT_FILELIST_FOCUSCHANGE, FileListCtrl::OnProcessFocusChanged)
EVT_LIST_ITEM_SELECTED(wxID_ANY, FileListCtrl::OnItemSelected)
EVT_LIST_ITEM_DESELECTED(wxID_ANY, FileListCtrl::OnItemDeselected)
EVT_KEY_DOWN(FileListCtrl::OnKeyDown)
wxEND_EVENT_TABLE();
// clang-format on

namespace {
constexpr const int column_widths[] = {272, 88, 96, 122};
std::unique_ptr<wxListItemAttr> CreateItemAttrWithBackground(
        const wxColor &color) {
    std::unique_ptr<wxListItemAttr> attr(new wxListItemAttr);
    attr->SetBackgroundColour(color);
    return attr;
}

} // namespace

FileListCtrl::FileListCtrl(wxWindow *parent,
                           wxWindowID winid,
                           const wxPoint &pos,
                           const wxSize &size)
    : wxListCtrl(parent, winid, pos, size, wxLC_REPORT | wxLC_VIRTUAL) {
#ifndef __WXMSW__
    dropHighlightAttribute_.SetBackgroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
#endif
    systemImageList_.CreateSystemImageList(
            ThemeProvider::GetIconSize(iconSizeSmall).x);
    SetImageList(systemImageList_.imageList(), wxIMAGE_LIST_SMALL);
    InitCompareItemColors();
}

void FileListCtrl::InitCompareItemColors() {
    ItemAttrNew_.SetBackgroundColour(*wxGREEN);
    ItemAttrUpdated_.SetBackgroundColour(*wxYELLOW);
    ItemAttrOutdated_.SetBackgroundColour(*wxLIGHT_GREY);
    ItemAttrConflict_.SetBackgroundColour(*wxRED);
}

void FileListCtrl::SiteUpdated(Site *site) {
    SetItemCount(site_->GetFiles().size());
    for (int i = 0; i < GetItemCount(); i++) {
        inSetState_ = true;
        SetItemState(i, 0, wxLIST_STATE_SELECTED);
        inSetState_ = false;
    }
    lastFocusItem_ = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
    selectedDirCount_ = 0;
    selectedFileCount_ = 0;
    selectedTotalSize_ = 0;
    selections_.clear();
    selections_.resize(site_->GetFiles().size());
    Update();
}

void FileListCtrl::SetFileColumns() {
    ClearAll();
    const int widths[4] = {272, 88, 96, 122};
    AppendColumn(_("name"), wxLIST_FORMAT_LEFT, widths[0]);
    AppendColumn(_("size"), wxLIST_FORMAT_RIGHT, widths[1]);
    AppendColumn(_("type"), wxLIST_FORMAT_LEFT, widths[2]);
    AppendColumn(_("time"), wxLIST_FORMAT_LEFT, widths[3]);
}

wxString FileListCtrl::OnGetItemText(long item, long column) const {
    const auto &file = site_->GetFiles()[item];
    if (column == 0) {
        return file->name;
    }
    if (column == 1) {
        if (file->type == FTFile) {
            return filesizefmt(file->stat.size);
        }
    }
    if (column == 2) {
        return fileTypeToStdWstring(file->type);
    }
    if (column == 3) {
        return opstrftimew(file->stat.lastModifiedTime);
    }
    return "";
}

int FileListCtrl::OnGetItemImage(long item) const {
    const auto &file = site_->GetFiles()[item];
    if (file->icon != -2) {
        return file->icon;
    }
    auto *self = const_cast<FileListCtrl *>(this);
    IconType iconType =
            file->type == FTDirectory ? IconType::dir : IconType::file;
    int icon = self->systemImageList_.GetIconIndex(iconType, file->name);
    file->icon = icon;
    return icon;
}

wxListItemAttr *FileListCtrl::OnGetItemAttr(long item) const {
#ifndef __WXMSW__
    FileListCtrl *self = const_cast<FileListCtrl *>(this);
    if (item == dropHighlightItem_) {
        return &self->dropHighlightAttribute_;
    }
#endif
    const auto &dir = site_->GetCurrentDir();
    if (!dir->compareEnable || dir->compareRunning) {
        return nullptr;
    }
    const auto &file = site_->GetFiles()[item];
    switch (file->cmp) {
    case CSEqual:
        return nullptr;
    case CSNew:
        return &self->ItemAttrNew_;
    case CSUpdated:
        return &self->ItemAttrUpdated_;
    case CSOutdated:
        return &self->ItemAttrOutdated_;
    case CSConflict:
        return &self->ItemAttrConflict_;
    default:
        return nullptr;
    }
}

std::string FileListCtrl::GetCurrentPath() const {
    return site_->GetCurrentPath();
}

const FilePtr &FileListCtrl::GetData(long item) const {
    return site_->GetFiles()[item];
}

std::string FileListCtrl::GetFileType(const std::string &file) {
    size_t pos = file.rfind('.');
    // if no dot or starts/ends with dot
    if (pos == std::string::npos || pos < 1 || !file[pos + 1]) {
        return "文件";
    }

    std::string ext = file.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto typeIt = fileTypeMap_.find(ext);
    if (typeIt != fileTypeMap_.end()) {
        return typeIt->second;
    }

    std::unique_ptr<wxFileType> pType(
            wxTheMimeTypesManager->GetFileTypeFromExtension(ext));
    if (!pType) {
        return ext;
    }
    wxString desc;
    if (pType->GetDescription(&desc) && !desc.empty()) {
        return fileTypeMap_[ext] = desc.ToStdString();
    }

    return "文件";
}

void FileListCtrl::OnSize(wxSizeEvent &event) {
    if (GetColumnCount() > 0) {
        int clientWidth = GetClientSize().GetWidth();
        if (clientWidth < 580) {
            SetColumnWidth(0, column_widths[0]);
        } else {
            SetColumnWidth(0,
                           clientWidth - column_widths[1] - column_widths[2] -
                                   column_widths[3]);
        }
    }
    event.Skip();
}

void FileListCtrl::OnLeftDown(wxMouseEvent &event) {
    event.Skip();
    wxPoint point = event.GetPosition();
    int flags;
    long item = HitTest(point, flags);
    if (item == -1) {
        SelectionCleared();
        NotifySelectionUpdated();
    }
}

void FileListCtrl::OnFocusChanged(wxListEvent &event) {
    long focusItem = event.GetIndex();
    if (focusItem >= 0 && focusItem < site_->GetFiles().size()) {
        wxCommandEvent *evt = new wxCommandEvent;
        evt->SetEventType(op_EVT_FILELIST_FOCUSCHANGE);
        evt->SetExtraLong(focusItem);
        QueueEvent(evt);
    } else {
        lastFocusItem_ = -1;
        SelectionCleared();
        NotifySelectionUpdated();
    }
}

void FileListCtrl::OnProcessFocusChanged(wxCommandEvent &event) {
    long focusItem = event.GetExtraLong();
    long lastFocusItem = lastFocusItem_;
    lastFocusItem_ = focusItem;

    if (lastFocusItem == -1) {
        SelectionUpdated(focusItem, focusItem);
    } else {
        SelectionUpdated(std::min(lastFocusItem, focusItem),
                         std::max(lastFocusItem, focusItem));
    }
}

void FileListCtrl::SelectionCleared() {
    for (size_t i = 0; i < selections_.size(); i++) {
        selections_[i] = false;
    }
    selectedDirCount_ = 0;
    selectedFileCount_ = 0;
    selectedTotalSize_ = 0;
}

void FileListCtrl::SelectionUpdated(int min, int max) {
    if (min < 0) {
        min = 0;
    }
    if (max >= GetItemCount()) {
        max = GetItemCount() - 1;
    }
    for (int i = min; i <= max; i++) {
        bool selected =
                GetItemState(i, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED;
        if (selected == selections_[i]) {
            continue;
        }
        selections_[i] = selected;
        const auto &file = site_->GetFiles()[i];
        if (selected) {
            if (file->type == FTDirectory) {
                if (file->name != "..") {
                    selectedDirCount_++;
                }
            } else {
                selectedFileCount_++;
                selectedTotalSize_ += file->stat.size;
            }
        } else {
            if (file->type == FTDirectory) {
                if (file->name != "..") {
                    selectedDirCount_--;
                }
            } else {
                selectedFileCount_--;
                selectedTotalSize_ -= file->stat.size;
            }
        }
    }
    NotifySelectionUpdated();
}

void FileListCtrl::NotifySelectionUpdated() {
    for (auto *l : listeners_) {
        l->SelectionUpdated(this);
    }
}

void FileListCtrl::OnItemSelected(wxListEvent &event) {
    if (inSetState_) {
        return;
    }

    if (GetSelectedItemCount() == 1) {
        SelectionCleared();
    }

    long item = event.GetIndex();
    if (!selections_[item]) {
        selections_[item] = true;
        const auto &file = site_->GetFiles()[item];
        if (file->type == FTDirectory) {
            if (file->name != "..") {
                selectedDirCount_++;
            }
        } else {
            selectedFileCount_++;
            selectedTotalSize_ += file->stat.size;
        }
        NotifySelectionUpdated();
    }
}

void FileListCtrl::OnItemDeselected(wxListEvent &event) {
    if (inSetState_) {
        return;
    }
    long item = event.GetIndex();
    if (selections_[item]) {
        selections_[item] = false;
        const auto &file = site_->GetFiles()[item];
        if (file->type == FTDirectory) {
            if (file->name != "..") {
                selectedDirCount_--;
            }
        } else {
            selectedFileCount_--;
            selectedTotalSize_ -= file->stat.size;
        }
    }
}

void FileListCtrl::OnKeyDown(wxKeyEvent &event) {
#ifdef __WXMAC__
#define CursorModifierKey wxMOD_CMD
#else
#define CursorModifierKey wxMOD_ALT
#endif
    const int code = event.GetKeyCode();
    const int mods = event.GetModifiers();

    if (code == 'A' &&
        (mods & wxMOD_CMD || (mods & (wxMOD_CONTROL | wxMOD_META)) ==
                                     (wxMOD_CONTROL | wxMOD_META))) {
        for (int i = 0; i < GetItemCount(); i++) {
            const auto &file = site_->GetFiles()[i];
            inSetState_ = true;
            if (file->type == FTDirectory && file->name == "..") {
                SetItemState(i, 0, wxLIST_STATE_SELECTED);
                selections_[i] = false;
            } else {
                SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                selections_[i] = true;
            }
            inSetState_ = false;
        }
        selectedDirCount_ = site_->GetCurrentDir()->dirCount;
        selectedFileCount_ = site_->GetCurrentDir()->fileCount;
        selectedTotalSize_ = site_->GetCurrentDir()->totalSize;
        NotifySelectionUpdated();
    } else if (code == 'L' && (mods & wxMOD_CMD)) {
        if (uriBox_) {
            uriBox_->SetFocus();
        }
    } else if (code == 'R' && (mods & wxMOD_CMD)) {
        if (explorer_) {
            explorer_->RefreshFileItems();
        }
    } else if (code == WXK_TAB) {
        if (peer_) {
            peer_->SetFocus();
        }
    } else if (code == WXK_F5) {
        if (explorer_) {
            explorer_->RefreshFileItems();
        }
    } else if (code == WXK_BACK && event.GetModifiers() != CursorModifierKey ||
               (code == WXK_LEFT &&
                event.GetModifiers() == CursorModifierKey)) {
        site_->Backward();
    } else if (code == WXK_RIGHT && event.GetModifiers() == CursorModifierKey) {
        site_->Forward();
    } else if (code == WXK_UP && event.GetModifiers() == CursorModifierKey) {
        site_->Up();
    } else if (code == WXK_DOWN && event.GetModifiers() == CursorModifierKey) {
        // do nothing
    } else if (code == WXK_BACK && event.GetModifiers() == CursorModifierKey) {
        RemoveFiles();
    } else {
        // keyModifier_ = event.GetModifiers();
        event.Skip();
    }
}

void FileListCtrl::ClearDropHighlight() {
    if (dropHighlightItem_ != -1) {
        SetItemState(dropHighlightItem_, 0, wxLIST_STATE_DROPHILITED);
        RefreshItem(dropHighlightItem_);
        dropHighlightItem_ = -1;
    }
}

void FileListCtrl::DisplayDropHighlight() {
    SetItemState(dropHighlightItem_,
                 wxLIST_STATE_DROPHILITED,
                 wxLIST_STATE_DROPHILITED);
    RefreshItem(dropHighlightItem_);
}

void FileListCtrl::OnActivated(wxListEvent &event) {
    long item = event.m_itemIndex;
    const auto &file = site_->GetFiles()[item];
    if (file->type == FTDirectory) {
        if (file->name == "..") {
            if (explorer_) {
                explorer_->Up();
            } else {
                site_->Up();
            }
        } else {
            std::string path = site_->GetCurrentPath() + file->name + "/";
            if (explorer_) {
                explorer_->willChangeDir(site_.get(), path);
            }
            site_->ChangeDir(path);
        }
    } else {
        if (explorer_) {
            explorer_->Copy();
        }
    }
}

void FileListCtrl::OnContextMenu(wxContextMenuEvent &event) {
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

void FileListCtrl::OnAdd(wxCommandEvent &event) {
    CreateDirectoryDialog dlg(site_.get(), this);
    dlg.ShowModal();
}

void FileListCtrl::OnCopy(wxCommandEvent &event) {
    if (explorer_) {
        explorer_->Copy();
    }
}

void FileListCtrl::OnDelete(wxCommandEvent &event) {
    RemoveFiles();
}

void FileListCtrl::ShowContextMenu(const wxPoint &point, long item) {
    wxMenu menu;
    menu.Append(opID_COPY, _("Copy"));
    menu.Append(wxID_DELETE, _("Delete"));
    menu.AppendSeparator();
    menu.Append(wxID_ADD, _("Create Directory"));

    if (explorer_ && explorer_->InTwinMode() &&
        !explorer_->GetOssSite()->IsInBuckets()) {
        menu.Enable(opID_COPY, true);
    } else {
        menu.Enable(opID_COPY, false);
    }

    int index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (index == -1) {
        menu.Enable(opID_COPY, false);
        menu.Enable(wxID_DELETE, false);
    } else {
        const auto &file = site_->GetFiles()[index];
        if (file->type == FTDirectory && file->name == "..") {
            menu.Enable(opID_COPY, false);
            menu.Enable(wxID_DELETE, false);
        }
    }

    PopupMenu(&menu, point.x, point.y);
}

void FileListCtrl::RemoveFiles() {
    std::vector<std::string> items;
    long item = -1;
    for (;;) {
        item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1) {
            break;
        }
        const auto &file = site_->GetFiles()[item];
        if (file->type == FTDirectory && file->name == "..") {
            continue;
        }
        std::string path = Site::Combine(site_->GetCurrentPath(), file);
        items.push_back(path);
    }
    if (!items.empty()) {
        RemoveObjectsDialog *dlg =
                new RemoveObjectsDialog(site_.get(), items, this);
        dlg->ShowModal();
        dlg->Destroy();
    }
}

void FileListCtrl::AddListener(FileListCtrlListener *l) {
    if (std::find(listeners_.begin(), listeners_.end(), l) ==
        listeners_.end()) {
        listeners_.push_back(l);
    }
}

void FileListCtrl::RemoveListener(FileListCtrlListener *l) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), l),
                     listeners_.end());
}

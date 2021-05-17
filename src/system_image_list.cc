#include "system_image_list.h"
#include "theme_provider.h"

#include <wx/mimetype.h>

#include <filesystem>

namespace {
void Overlay(wxBitmap &bg, const wxBitmap &fg) {}

void OverlaySymlink(wxBitmap &bmp) {
    wxBitmap overlay = themeProvider()->CreateBitmap(
            "ART_SYMLINK", wxART_OTHER, bmp.GetScaledSize());
    Overlay(bmp, overlay);
}

wxBitmap PrepareIcon(wxIcon icon, wxSize size) {
    if (icon.GetWidth() == size.GetWidth() &&
        icon.GetHeight() == size.GetHeight()) {
        return icon;
    }
    wxBitmap bmp;
    bmp.CopyFromIcon(icon);
    return bmp.ConvertToImage().Rescale(size.GetWidth(), size.GetHeight());
}
} // namespace

SystemImageList::SystemImageList() {}

SystemImageList::~SystemImageList() {
    if (imageList_) {
        delete imageList_;
    }
}

bool SystemImageList::CreateSystemImageList(int size) {
    if (imageList_) {
        return true;
    }

    imageList_ = new wxImageList(size, size);
    wxBitmap file = themeProvider()->CreateBitmap(
            "ART_FILE", wxART_OTHER, wxSize(size, size));
    wxBitmap folderclosed = themeProvider()->CreateBitmap(
            "ART_FOLDERCLOSED", wxART_OTHER, wxSize(size, size));
    wxBitmap folder = themeProvider()->CreateBitmap(
            "ART_FOLDER", wxART_OTHER, wxSize(size, size));
    imageList_->Add(file);
    imageList_->Add(folderclosed);
    imageList_->Add(folder);

    OverlaySymlink(file);
    OverlaySymlink(folderclosed);
    OverlaySymlink(folder);

    imageList_->Add(file);
    imageList_->Add(folderclosed);
    imageList_->Add(folder);

    wxBitmap bucket = themeProvider()->CreateBitmap(
            "ART_BUCKET", wxART_OTHER, wxSize(size, size));
    imageList_->Add(bucket);

    return true;
}

int SystemImageList::GetIconIndex(
        IconType iconType, const std::string &file, bool symlink) {
    if (!imageList_) {
        return -1;
    }

    int icon;
    switch (iconType) {
    case IconType::file:
    default:
        icon = symlink ? 3 : 0;
        break;
    case IconType::dir:
        return symlink ? 4 : 1;
    }

    std::string ext = std::filesystem::path(file).extension();
    if (ext.empty() || ext == ".") {
        return icon;
    }

    auto iconIt = iconCache_.find(std::make_pair(ext, symlink));
    if (iconIt != iconCache_.end()) {
        return iconIt->second;
    }

    std::unique_ptr<wxFileType> pType(
            wxTheMimeTypesManager->GetFileTypeFromExtension(ext));
    if (!pType) {
        iconCache_[std::make_pair(ext, true)] = icon;
        iconCache_[std::make_pair(ext, false)] = icon;
        return icon;
    }
    wxIconLocation loc;
    if (pType->GetIcon(&loc) && loc.IsOk()) {
        wxIcon typeIcon(loc);
        if (typeIcon.Ok()) {
            wxBitmap bmp = PrepareIcon(
                    typeIcon, ThemeProvider::GetIconSize(iconSizeSmall));
            if (symlink) {
                OverlaySymlink(bmp);
            }
            int index = imageList_->Add(bmp);
            if (index > 0) {
                icon = index;
            }
        }
    }
    iconCache_[std::make_pair(ext, symlink)] = icon;
    return icon;
}

#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/imaglist.h>

#include <wx/imaglist.h>

#include <map>

enum class IconType { file, dir };

class SystemImageList {
public:
    SystemImageList();
    virtual ~SystemImageList();

    bool CreateSystemImageList(int size);

    int GetIconIndex(
            IconType iconType, const std::string &file, bool symlink = false);

    wxImageList *imageList() const { return imageList_; }

private:
    wxImageList *imageList_{};
    // ext, symlink -> icon index
    std::map<std::pair<std::string, bool>, int> iconCache_;

};

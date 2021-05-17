#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/artprov.h>

#include <map>

enum IconSize { iconSizeTiny, iconSizeSmall };

class Theme final {
public:
    bool Load(const std::string &theme);
    struct size_cmp final {
        bool operator()(const wxSize &a, const wxSize &b) const {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        }
    };

    struct CacheEntry {
        std::map<wxSize, wxBitmap, size_cmp> bitmaps_;
        std::map<wxSize, wxImage, size_cmp> images_;
    };

    const wxBitmap &LoadBitmap(const std::string &name, const wxSize &size);
    const wxBitmap &DoLoadBitmap(
            const std::string &name, const wxSize &size, CacheEntry &cache);
    const wxBitmap &LoadBitmapWithSpecificSizeAndScale(
            const std::string &name,
            const wxSize &size,
            const wxSize &scale,
            CacheEntry &cache);
    const wxImage &LoadImageWithSpecificSize(
            const std::string &file, const wxSize &size, CacheEntry &cache);

private:
    std::string theme_;
    std::string path_;
    std::map<wxSize, bool, size_cmp> sizes_;
    std::map<std::string, CacheEntry> cache_;
};

class ThemeProvider final : public wxArtProvider {
public:
    ThemeProvider();
    virtual ~ThemeProvider();

    static wxSize GetIconSize(IconSize type, bool userScaled = false);

    wxBitmap CreateBitmap(
            const wxArtID &artId,
            const wxArtClient &client,
            const wxSize &size) override;

private:
    std::map<std::string, Theme> themes_;
};

ThemeProvider *themeProvider();

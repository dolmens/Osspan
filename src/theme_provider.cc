#include "theme_provider.h"

#include <wx/stdpaths.h>

#include <yaml-cpp/yaml.h>

#include <sstream>

namespace {
wxSize stringToSize(const std::string &str) {
    wxSize size;
    size_t pos = str.find('x');
    if (pos != str.npos) {
        size.SetWidth(std::stoi(str.substr(0, pos)));
        size.SetHeight(std::stoi(str.substr(pos+1)));
    }
    return size;
}
}

bool Theme::Load(const std::string &theme) {
    theme_ = theme;
    std::string dataPath =
            wxStandardPaths::Get().GetDataDir().ToStdString();
    path_ = dataPath + "/" + theme + "/";
    YAML::Node themeRootNode = YAML::LoadFile(path_ + "theme.yml");
    for (const auto &sizeNode : themeRootNode["sizes"]) {
        wxSize size = stringToSize(sizeNode.as<std::string>());
        if (size.x > 0 && size.y > 0) {
            sizes_[size] = true;
        }
    }
    return !sizes_.empty();
}

const wxBitmap &Theme::LoadBitmap(const std::string &name, const wxSize &size) {
    auto it = cache_.find(name);
    if (it == cache_.end()) {
        it = cache_.insert(std::make_pair(name, CacheEntry())).first;
    } else {
        if (it->second.bitmaps_.empty()) {
            // the name has been loaded but image does not exist
            static wxBitmap empty;
            return empty;
        }
    }
    auto &bitmaps = it->second.bitmaps_;
    auto bit = bitmaps.find(size);
    if (bit != bitmaps.end()) {
        return bit->second;
    }

    return DoLoadBitmap(name, size, it->second);
}

const wxBitmap &Theme::DoLoadBitmap(
        const std::string &name, const wxSize &size, CacheEntry &cache) {
    wxSize pivotSize = size;
#ifdef __WXMAC__
    double scale = wxTheApp->GetTopWindow()->GetContentScaleFactor();
    pivotSize.Scale(scale, scale);
#endif

    // First look equal or larger ones
    const auto pivot = sizes_.lower_bound(pivotSize);
    for (auto pit = pivot; pit != sizes_.end(); ++pit) {
        const wxBitmap &bmp = LoadBitmapWithSpecificSizeAndScale(
                name, pit->first, size, cache);
        if (bmp.IsOk()) {
            return bmp;
        }
    }

    // Then look smaller ones
    for (auto pit = decltype(sizes_)::reverse_iterator(pivot);
         pit != sizes_.rend();
         ++pit) {
        const wxBitmap &bmp = LoadBitmapWithSpecificSizeAndScale(
                name, pit->first, size, cache);
        if (bmp.IsOk()) {
            return bmp;
        }
    }

    // Out of luck
    static wxBitmap empty;
    return empty;
}

const wxBitmap &Theme::LoadBitmapWithSpecificSizeAndScale(
        const std::string &name,
        const wxSize &size,
        const wxSize &scale,
        CacheEntry &cache) {
    std::ostringstream filess;
    filess << path_ << size.x << "x" << size.y << "/" << name << ".png";
    std::string file = filess.str();
    wxImage image = LoadImageWithSpecificSize(file, size, cache);
    if (!image.IsOk()) {
        static wxBitmap empty;
        return empty;
    }
    double factor = static_cast<double>(image.GetSize().x) / scale.x;
    auto inserted = cache.bitmaps_.insert(
            std::make_pair(scale, wxBitmap(image, -1, factor)));
    return inserted.first->second;
}

const wxImage &Theme::LoadImageWithSpecificSize(
        const std::string &file, const wxSize &size, CacheEntry &cache) {
    auto it = cache.images_.find(size);
    if (it != cache.images_.end()) {
        return it->second;
    }

    wxImage img(file, wxBITMAP_TYPE_PNG);
    if (img.Ok()) {
        if (img.HasMask() && !img.HasAlpha()) {
            img.InitAlpha();
        }
        if (img.GetSize() != size) {
            img.Rescale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
        }
    }
    auto inserted = cache.images_.insert(std::make_pair(size, img));
    return inserted.first->second;
}

ThemeProvider *themeProvider() {
    static ThemeProvider *inst = new ThemeProvider;
    return inst;
}

ThemeProvider::ThemeProvider() {
    wxArtProvider::Push(this);
    Theme defaultTheme;
    if (defaultTheme.Load("default")) {
        themes_["default"] = defaultTheme;
    }
}

ThemeProvider::~ThemeProvider() {}

wxBitmap ThemeProvider::CreateBitmap(
        const wxArtID &artId, const wxArtClient &client, const wxSize &size) {
    if (artId.Left(4) != _T("ART_")) {
        return wxNullBitmap;
    }
    wxASSERT(size.GetWidth() == size.GetHeight());

    wxSize nsize = size;
    if (nsize.x <= 0 || nsize.y <= 0) {
        nsize = GetNativeSizeHint(client);
        if (nsize.x <= 0 || nsize.y <= 0) {
            nsize = GetIconSize(iconSizeSmall);
        }
    }

    std::string name = artId.substr(4).ToStdString();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    const wxBitmap *bmp{&wxNullBitmap};
    auto tryTheme = [&bmp, &name, &nsize, this](const std::string &theme) {
        if (!bmp->IsOk()) {
            auto it = this->themes_.find(theme);
            if (it != this->themes_.end()) {
                bmp = &it->second.LoadBitmap(name, nsize);
            }
        }
    };

    tryTheme("default");

    return *bmp;
}

wxSize ThemeProvider::GetIconSize(IconSize iconSize, bool userScaled) {
    int s;
    if (iconSize == iconSizeTiny) {
        s = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X) * 3 / 4;
        if (s <= 0) {
            s = 12;
        }
    } else if (iconSize == iconSizeSmall) {
        s = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
        if (s <= 0) {
            s = 16;
        }
    } else {
        s = wxSystemSettings::GetMetric(wxSYS_ICON_X);
        if (s <= 0) {
            s = 32;
        }
    }

    wxSize sz(s, s);

    if (userScaled) {
    }

    return sz;
}

#include "string_format.h"

wxString opstrftimew(std::time_t t) {
    if (t == 0) {
        return _T("");
    }

    std::time_t now = std::time(nullptr);
    std::tm nowlt;
    localtime_r(&now, &nowlt);
    std::tm lt;
    localtime_r(&t, &lt);

    if (lt.tm_year == nowlt.tm_year) {
        if (lt.tm_mon == nowlt.tm_mon) {
            if (lt.tm_mday == nowlt.tm_mday) {
                return wxString::Format(_T("%02d:%02d"), lt.tm_hour, lt.tm_min);
            }
            if (lt.tm_mday + 1 == nowlt.tm_mday) {
                return wxString::Format(
                        _T("昨天 %02d:%02d"), lt.tm_hour, lt.tm_min);
            }
            if (lt.tm_mday + 2 == nowlt.tm_mday) {
                return wxString::Format(
                        _T("前天 %02d:%02d"), lt.tm_hour, lt.tm_min);
            }
        }
        return wxString::Format(_T("%d月%d日 %02d:%02d"),
                                lt.tm_mon + 1,
                                lt.tm_mday,
                                lt.tm_hour,
                                lt.tm_min);
    }
    return wxString::Format(_T("%d年%d月%d日"),
                            lt.tm_year + 1900,
                            lt.tm_mon + 1,
                            lt.tm_mday);
}

wxString filesizefmt(size_t sz) {
    if (sz < 1000) {
        return wxString::Format(_T("%zu\u202F%s"), sz, _("Bytes"));
    }

    if (sz < 999.5 * 1000) {
        sz = std::round(sz / 1000.0);
        return wxString::Format(_T("%zu\u202FKB"), sz);
    }

    if (sz < 999.95 * 1000 * 1000) {
        sz = std::round(sz / (1000 * 100.0));
        return wxString::Format(_T("%.15g\u202FMB"), (sz / 10.0));
    }

    sz = std::round(sz / (1000 * 1000 * 10.0));
    return wxString::Format(_T("%.15g\u202FGB"), (sz / 100.0));
}

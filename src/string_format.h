#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <ctime>

wxString opstrftimew(std::time_t time);

wxString filesizefmt(size_t sz);

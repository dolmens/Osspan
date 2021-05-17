#pragma once

#include <wx/defs.h>

#define wxDECLARE_EVENT_TABLEex()                                         \
    _Pragma("GCC diagnostic push")                                        \
    _Pragma("GCC diagnostic ignored \"-Winconsistent-missing-override\"") \
    wxDECLARE_EVENT_TABLE()                                               \
    _Pragma("GCC diagnostic pop")

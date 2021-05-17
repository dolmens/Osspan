#pragma once

#include "file_list_view.h"
#include "local_list_ctrl.h"
#include "local_site.h"
#include "local_uri_box.h"

class LocalListView
    : public FileListView<LocalSite, LocalUriBox, LocalListCtrl> {
public:
    using FileListView<LocalSite, LocalUriBox, LocalListCtrl>::FileListView;
};

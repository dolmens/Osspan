#pragma once

#include "file_list_view.h"
#include "oss_list_ctrl.h"
#include "oss_site.h"
#include "oss_uri_box.h"

class OssListView : public FileListView<OssSite, OssUriBox, OssListCtrl> {
public:
    using FileListView<OssSite, OssUriBox, OssListCtrl>::FileListView;
};

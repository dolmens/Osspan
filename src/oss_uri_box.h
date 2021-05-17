#pragma once

#include "uri_box.h"

class OssUriBox : public UriBox {
public:
    using UriBox::UriBox;
    std::string Format() override;
};

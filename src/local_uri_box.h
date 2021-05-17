#pragma once

#include "uri_box.h"

class LocalUriBox : public UriBox {
public:
    using UriBox::UriBox;
    std::string Format() override;
};

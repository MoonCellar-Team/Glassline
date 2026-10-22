#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ImageClass : public midp::Class {
public:
    ImageClass();
    virtual ~ImageClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}

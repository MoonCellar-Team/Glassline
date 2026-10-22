#pragma once

#include "runtime/midp/Class.h"

class BGMusicClass : public midp::Class {
public:
    BGMusicClass();
    virtual ~BGMusicClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

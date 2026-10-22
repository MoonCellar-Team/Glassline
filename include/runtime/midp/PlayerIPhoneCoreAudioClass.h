#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class PlayerIPhoneCoreAudioClass : public midp::Class {
public:
    PlayerIPhoneCoreAudioClass();
    virtual ~PlayerIPhoneCoreAudioClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}

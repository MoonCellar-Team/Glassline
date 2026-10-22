#pragma once
#include "m3g/Object3D.h"
namespace midp { class InputStream; }
namespace m3g {
class Loader {
public:
    static midp::array<Object3D*> load(midp::InputStream* input);
    static const unsigned char M3G_FILE_IDENTIFIER[12];
};
}

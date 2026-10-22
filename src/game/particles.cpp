#include "game/particles.h"

namespace particles {

void colorFloatsToBytes(float* rgba, signed char* out) {
    for (int i = 0; i < 4; i++) {
        out[i] = (signed char)(int)(rgba[i] * 255.0f);
    }
}

}

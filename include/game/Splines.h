#pragma once

#include "game/MathVector.h"

namespace Splines {

MathVector getPositionOnCubic(const MathVector& p0, const MathVector& m0, const MathVector& p1,
                              const MathVector& m1, float t);

}

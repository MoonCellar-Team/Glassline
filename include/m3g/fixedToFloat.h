#pragma once

namespace m3g {

struct fixedToFloat {
    float operator()(int value);
};

}

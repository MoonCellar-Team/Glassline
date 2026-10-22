#pragma once
namespace m3g {
struct halfToFloat {
    float operator()(short value);
};
}

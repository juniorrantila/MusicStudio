#pragma once
#include <LibTy/Base.h>

namespace Vst {

struct Rectangle {
    i16 y;      // Y-value in pixels of top.
    i16 x;      // X-value in pixels of left.
    i16 height; // Y-value in pixels of bottom.
    i16 width;  // X-value in pixels of right.
};

}

#pragma once
#include <Basic/Base.h>

#include "./Layout.h"

DEFINE_MESSAGE(LayoutRenderSetResolution) {
    f32 width;
    f32 height;
};

DEFINE_MESSAGE(LayoutRenderFlush) {
};

DEFINE_MESSAGE(LayoutRenderRectangle) {
    LayoutBox bounding_box;
    LayoutColor color;
};

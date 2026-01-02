#pragma once
#include <Basic/Base.h>

#include "./Layout.h"

static constexpr THMessageKind LayoutRenderCommandKind_SetResolution = TH_MESSAGE_KIND("la.sr");
static constexpr THMessageKind LayoutRenderCommandKind_Flush = TH_MESSAGE_KIND("la.f");
static constexpr THMessageKind LayoutRenderCommandKind_Rectangle = TH_MESSAGE_KIND("la.r");

typedef struct LayoutRenderSetResolution {
    IF_CPP(static constexpr THMessageKind kind = LayoutRenderCommandKind_SetResolution;)
    f32 width;
    f32 height;
} LayoutRenderSetResolution;

typedef struct LayoutRenderFlush {
    IF_CPP(static constexpr THMessageKind kind = LayoutRenderCommandKind_Flush;)
} LayoutRenderFlush;

typedef struct LayoutRenderRectangle {
    IF_CPP(static constexpr THMessageKind kind = LayoutRenderCommandKind_Rectangle;)
    LayoutBox bounding_box;
    LayoutColor color;
    u32 debug_id;
} LayoutRenderRectangle;

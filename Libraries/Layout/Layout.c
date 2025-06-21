#include "./Layout.h"

#include <Clay/Clay.h>
#include <NanoVG/NanoVGL.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct Layout {
    NVGcontext* nvg;
    void (*on_error)(void* user, char const* message, usize size);
    void* on_error_user;

    f32 width;
    f32 height;
    f32 pixel_ratio;

    u8 clay[];
} Layout;

static void clay_handle_error(Clay_ErrorData error);
static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig *config, uintptr_t user);

Layout* layout_create(void* on_error_user, void(*on_error)(void* user, char const*, usize))
{
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Layout* layout = calloc(sizeof(Layout) + totalMemorySize, 1);
    if (!layout) {
        return 0;
    }
    NVGcontext* nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (!nvg) {
        free(layout);
        return 0;
    }
    *layout = (Layout) {
        .nvg = nvg,
        .on_error = on_error,
        .on_error_user = on_error_user,
    };
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, &layout->clay);
    Clay_Initialize(arena, (Clay_Dimensions){ 0, 0 }, (Clay_ErrorHandler) {
        .errorHandlerFunction = clay_handle_error,
        .userData = (uptr)layout,
    });

    Clay_SetMeasureTextFunction(measure_text, (uptr)layout);

    return layout;
}

void layout_destroy(Layout* layout)
{
    nvgDeleteGL3(layout->nvg);
    free(layout);
}

int layout_add_typeface(Layout* layout, c_string name, u8 const* data, usize data_size)
{
    return nvgCreateFontMem(layout->nvg, name, (u8*)data, data_size, 0);
}

void layout_update_scroll(Layout* layout, Vec2f scroll, f32 delta_time)
{
    (void)layout;
    Clay_UpdateScrollContainers(
        false,
        (Clay_Vector2){ scroll.x, scroll.y },
        delta_time
    );
}

void layout_set_pointer_state(Layout* layout, Vec2f position, bool left_down)
{
    (void)layout;
    Clay_SetPointerState((Clay_Vector2){ position.x, position.y }, left_down);
}

void layout_set_size(Layout* layout, Vec2f size, f32 pixel_ratio)
{
    (void)layout;
    layout->width = size.x;
    layout->height = size.y;
    layout->pixel_ratio = pixel_ratio;
    Clay_SetLayoutDimensions((Clay_Dimensions) { size.x, size.y });
}

void layout_render(Layout* layout, Clay_RenderCommandArray commands)
{
    nvgBeginFrame(layout->nvg, layout->width, layout->height, layout->pixel_ratio);
    for (i32 i = 0; i < commands.length; i++) {
        Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&commands, i);
        Clay_BoundingBox box = command->boundingBox;
        switch (command->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleElementConfig* config = command->config.rectangleElementConfig;
            Clay_Color color = config->color;
            Clay_CornerRadius corner = config->cornerRadius;
            nvgBeginPath(layout->nvg);
            nvgFillColor(layout->nvg, nvgRGBA(color.r, color.g, color.b, color.a));
            nvgRoundedRectVarying(layout->nvg, box.x, box.y, box.width, box.height, corner.topLeft, corner.topRight, corner.bottomRight, corner.bottomLeft);
            nvgFill(layout->nvg);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_TextElementConfig* config = command->config.textElementConfig;
            Clay_String text = command->text;
            Clay_Color color = config->textColor;
            nvgFontFaceId(layout->nvg, config->fontId);
            nvgFontSize(layout->nvg, config->fontSize);
            nvgTextLineHeight(layout->nvg, config->lineHeight);
            nvgTextLetterSpacing(layout->nvg, config->letterSpacing);
            nvgFillColor(layout->nvg, nvgRGBA(color.r, color.g, color.b, color.a));
            nvgTextAlign(layout->nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgText(layout->nvg, box.x, box.y, text.chars, text.chars + text.length);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            nvgScissor(layout->nvg, box.x, box.y, box.width, box.height);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
            nvgResetScissor(layout->nvg);
            break;
        }
        default: {
            fprintf(stderr, "Error: unhandled render command: %d\n", command->commandType);
            exit(1);
        }
    }
    }
    nvgEndFrame(layout->nvg);
}


static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, uintptr_t user)
{
    Layout* layout = (Layout*)user;
    nvgFontFaceId(layout->nvg, config->fontId);
    nvgFontSize(layout->nvg, config->fontSize);
    nvgTextLineHeight(layout->nvg, config->lineHeight);
    nvgTextLetterSpacing(layout->nvg, config->letterSpacing);
    nvgTextAlign(layout->nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    f32 bounds[4];
    nvgTextBounds(layout->nvg, 0, 0, text.chars, text.chars + text.length, bounds);
    return (Clay_Dimensions) {
        .width = bounds[2],
        .height = bounds[3],
    };
}

void layout_begin(Layout*)
{
    Clay_BeginLayout();
}

LayoutRenderCommands layout_end(Layout*)
{
    return Clay_EndLayout();
}

static void clay_handle_error(Clay_ErrorData error)
{
    Layout* layout = (Layout*)error.userData;
    if (layout->on_error) {
        layout->on_error(layout->on_error_user, error.errorText.chars, error.errorText.length);
    }
}


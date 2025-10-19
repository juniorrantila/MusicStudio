#pragma once
#include <Basic/Types.h>

#include <Clay/Clay.h>

typedef struct Layout Layout;

typedef Clay_RenderCommandArray LayoutRenderCommands;

C_API Layout* layout_create(void* on_error_user, void(*on_error)(void* user, char const* error, u64 size));
C_API void layout_destroy(Layout* layout);

C_API int layout_add_typeface(Layout* layout, c_string name, u8 const* data, u64 data_size);

C_API void layout_update_scroll(Layout* layout, v2 scroll, f32 delta_time);
C_API void layout_set_pointer_state(Layout* layout, v2 position, bool left_down);
C_API void layout_set_size(Layout* layout, v2 size, f32 pixel_ratio);
C_API void layout_render(Layout* layout, LayoutRenderCommands);

C_API void layout_begin(Layout* layout);
C_API LayoutRenderCommands layout_end(Layout* layout);

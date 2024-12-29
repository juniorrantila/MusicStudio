#pragma once
#include <Ty/Base.h>
#include <Rexim/LA.h>
#include <Clay/Clay.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Layout Layout;

Layout* layout_create(void* on_error_user, void(*on_error)(void* user, char const* error, usize size));
void layout_destroy(Layout* layout);

int layout_add_typeface(Layout* layout, c_string name, u8 const* data, usize data_size);

void layout_update_scroll(Layout* layout, Vec2f scroll, f32 delta_time);
void layout_set_pointer_state(Layout* layout, Vec2f position, bool left_down);
void layout_set_size(Layout* layout, Vec2f size, f32 pixel_ratio);
void layout_render(Layout* layout, Clay_RenderCommandArray);

#ifdef __cplusplus
}
#endif

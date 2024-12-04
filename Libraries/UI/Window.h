#pragma once
#include "./Forward.h"
#include <Ty/Base.h>
#include <Rexim/LA.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_KEYMAP_SIZE ((usize)1024)

typedef struct WSWindowSpec {
    UIWindow* parent;
    c_string title;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
} UIWindowSpec;

typedef struct UIMouseState {
    i32 left_down;
    i32 right_down;
} UIMouseState;

c_string ui_window_strerror(int);
UIWindow* ui_window_create(UIApplication*, UIWindowSpec);
void ui_window_destroy(UIWindow*);
void* ui_window_native_handle(UIWindow*);
void ui_window_close(UIWindow*);
bool ui_window_should_close(UIWindow*);
int ui_window_show(UIWindow*);
UIApplication* ui_window_application(UIWindow* window);

bool ui_window_is_fullscreen(UIWindow const* window);

Vec2f ui_window_size(UIWindow*);
int ui_window_set_resize_callback(UIWindow*, void* user, void(*)(UIWindow* window, void*));

u8 const* ui_window_keymap(UIWindow*);
Vec2f ui_window_mouse_pos(UIWindow*);
UIMouseState ui_window_mouse_state(UIWindow*);

void ui_window_gl_make_current_context(UIWindow*);
void ui_window_gl_flush(UIWindow*);

#ifdef __cplusplus
}
#endif

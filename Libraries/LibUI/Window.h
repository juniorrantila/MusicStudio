#pragma once
#include "./Forward.h"

#include <Basic/Types.h>

#define UI_KEYMAP_SIZE ((u64)1024)

typedef struct UIWindowSpec {
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

C_API c_string ui_window_strerror(int);
C_API UIWindow* ui_window_create(UIApplication*, UIWindowSpec);
C_API void ui_window_destroy(UIWindow*);
C_API void* ui_window_native_handle(UIWindow*);
C_API void ui_window_close(UIWindow*);
C_API bool ui_window_should_close(UIWindow*);
C_API int ui_window_show(UIWindow*);
C_API int ui_window_hide(UIWindow*);
C_API UIApplication* ui_window_application(UIWindow* window);
C_API f32 ui_window_pixel_ratio(UIWindow* window);

C_API bool ui_window_is_fullscreen(UIWindow const* window);

C_API v2 ui_window_size(UIWindow*);
C_API int ui_window_set_resize_callback(UIWindow*, void* user, void(*)(UIWindow* window, void*));

C_API v2 ui_window_scroll_delta(UIWindow*);
C_API int ui_window_set_scroll_callback(UIWindow*, void* user, void(*)(UIWindow* window, void*));

C_API u8 const* ui_window_keymap(UIWindow*);
C_API v2 ui_window_mouse_pos(UIWindow*);
C_API UIMouseState ui_window_mouse_state(UIWindow*);

C_API [[nodiscard]] bool ui_window_gl_init(UIWindow*, u8 major, u8 min);
C_API void ui_window_gl_make_current_context(UIWindow*);
C_API void ui_window_gl_flush(UIWindow*);

typedef struct MetalView MetalView;
C_API [[nodiscard]] MetalView* ui_window_metal_init(UIWindow*);

C_API void ui_window_autosave(UIWindow*, c_string name);

C_API void ui_window_set_resizable(UIWindow*, bool);
C_API void ui_window_set_size(UIWindow*, v2);

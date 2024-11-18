#pragma once
#include "./Forward.h"
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WSWindowSpec {
    UIWindow* parent;
    c_string title;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
} UIWindowSpec;

typedef struct UIMouseState {
    bool left_down;
    bool right_down;
} UIMouseState;

c_string ui_window_strerror(int);
UIWindow* ui_window_create(UIApplication*, UIWindowSpec);
void ui_window_destroy(UIWindow*);
void* ui_window_native_handle(UIWindow*);
void ui_window_close(UIWindow*);
bool ui_window_should_close(UIWindow*);
int ui_window_show(UIWindow*);

int ui_window_size(UIWindow*, i32* x, i32* y);

u8 const* ui_window_keymap(UIWindow*);
void ui_window_mouse_pos(UIWindow*, i32* x, i32* y);
UIMouseState ui_window_mouse_state(UIWindow*);


#ifdef __cplusplus
}
#endif

#pragma once
#include "./Forward.h"

#include <Render/Render.h>
#include <Rexim/LA.h>
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIState {
    Vec2f current_point;
    usize id;
    usize active_id;
} UIState;

typedef struct UI {
    UIWindow* window;
    Render* render;
    UIState state;
} UI;

UI ui_create(UIWindow* window, Render* render);

void ui_begin_frame(UI* ui);
void ui_end_frame(UI* ui);

bool ui_button(UI* ui, c_string label, Vec4f color);
void ui_spacer(UI* ui, Vec2f pad);

#ifdef __cplusplus
}
#endif

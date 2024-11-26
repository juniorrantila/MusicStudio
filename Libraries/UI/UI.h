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
    usize hover_id;
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
void ui_rect(UI* ui, Vec2f size, Vec4f color);

Vec2f ui_current_point(UI* ui);
void ui_move_point(UI* ui, Vec2f);

#ifdef __cplusplus
}
#endif

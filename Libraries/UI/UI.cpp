#include "./UI.h"
#include "./Window.h"

#include <string.h>

static Vec2f normalized(UI* ui, Vec2f point);

UI ui_create(UIWindow* window, Render* render)
{
    ui_window_gl_make_current_context(window);
    i32 width = 0;
    i32 height = 1;
    ui_window_size(window, &width, &height);
    render_set_resolution(render, vec2f(width, height) * 2.0);
    return UI {
        .window = window,
        .render = render,
        .state = UIState(),
    };
}

void ui_begin_frame(UI* ui)
{
    ui->state.id = 0;
    ui->state.current_point = vec2fs(0);

    ui_window_gl_make_current_context(ui->window);

    i32 width = 0;
    i32 height = 1;
    ui_window_size(ui->window, &width, &height);
    render_set_resolution(ui->render, vec2f(width, height) * 2.0);

    i32 mouse_x = 0;
    i32 mouse_y = 0;
    ui_window_mouse_pos(ui->window, &mouse_x, &mouse_y);
    render_set_mouse_position(ui->render, vec2f(mouse_x, mouse_y));
}

void ui_end_frame(UI* ui)
{
    render_flush(ui->render);
    ui_window_gl_flush(ui->window);
}

bool ui_button(UI* ui, c_string label, Vec4f color)
{
    usize text_size = strlen(label);
    auto id = ++ui->state.id;
    auto size = normalized(ui, vec2f(text_size * 16.0f, 32.0f));
    auto start = ui->state.current_point;
    auto end = start + size;
    ui->state.current_point.y = end.y;

    usize last_active = ui->state.active_id;
    bool click = false;
    i32 x = 0;
    i32 y = 0;
    ui_window_mouse_pos(ui->window, &x, &y);
    auto mouse = normalized(ui, vec2f(x, y));
    if (mouse.is_inside(vec4fv(start, size))) {
        color *= vec4fs(1.08);
        auto state = ui_window_mouse_state(ui->window);
        click = state.left_down;
        if (click) {
            color *= vec4fs(0.9);
            ui->state.active_id = id;
        } else {
            ui->state.active_id = 0;
        }
    }
    if (last_active == ui->state.active_id) {
        click = false;
    }

    render_quad(ui->render,
        vec2f(start.x, start.y), color, vec2fs(0),
        vec2f(end.x,   start.y), color, vec2fs(0),
        vec2f(start.x, end.y),   color, vec2fs(0),
        vec2f(end.x,   end.y),   color, vec2fs(0)
    );

    return click;
}

void ui_spacer(UI* ui, Vec2f pad)
{
    auto size = normalized(ui, pad);
    auto start = ui->state.current_point;
    auto end = start + size;
    ui->state.current_point = end;
}

static Vec2f normalized(UI* ui, Vec2f point)
{
    i32 x = 1;
    i32 y = 1;
    ui_window_size(ui->window, &x, &y);
    return point / vec2f(x, y);
}


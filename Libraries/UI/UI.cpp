#include "./UI.h"
#include "./Window.h"
#include "./Application.h"

#include <string.h>

static Vec2f normalized(UI* ui, Vec2f point);
static Vec2f to_screen(UI* ui, Vec2f point);

UI ui_create(UIWindow* window, Render* render)
{
    ui_window_gl_make_current_context(window);
    render_set_resolution(render, ui_window_size(window) * 2.0);
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
    ui->state.mode = UIMode_Below;

    ui_window_gl_make_current_context(ui->window);

    render_set_resolution(ui->render, ui_window_size(ui->window) * 2.0);
    render_set_mouse_position(ui->render, ui_window_mouse_pos(ui->window));
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
    if (ui->state.mode == UIMode_Below) {
        ui->state.current_point.y = end.y;
    } else {
        ui->state.current_point.x = end.x;
    }

    usize last_active = ui->state.active_id;
    bool click = false;
    auto mouse = normalized(ui, ui_window_mouse_pos(ui->window));

    auto* app = ui_window_application(ui->window);
    auto current_cursor = ui_application_cursor(app);
    if (mouse.is_inside(vec4fv(start, size))) {
        if (ui->state.hover_id != id) {
            if (current_cursor != UICursor_Pointer) {
                ui_application_cursor_push(app, UICursor_Pointer);
            }
            ui->state.hover_id = id;
        }

        color *= vec4fs(1.08);
        auto state = ui_window_mouse_state(ui->window);
        click = state.left_down;
        if (click) {
            color *= vec4fs(0.9);
            ui->state.active_id = id;
        } else {
            ui->state.active_id = 0;
        }
    } else {
        if (ui->state.hover_id == id) {
            if (current_cursor == UICursor_Pointer) {
                ui_application_cursor_pop(app);
            }
            ui->state.hover_id = 0;
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

void ui_rect(UI* ui, Vec2f size, Vec4f color)
{
    auto start = ui->state.current_point;
    auto end = start + normalized(ui, size);
    if (ui->state.mode == UIMode_Below) {
        ui->state.current_point.y = end.y;
    } else {
        ui->state.current_point.x = end.x;
    }
    render_quad(ui->render,
        vec2f(start.x, start.y), color, vec2fs(0),
        vec2f(end.x,   start.y), color, vec2fs(0),
        vec2f(start.x, end.y),   color, vec2fs(0),
        vec2f(end.x,   end.y),   color, vec2fs(0)
    );
}

Vec2f ui_current_point(UI* ui)
{
    return to_screen(ui, ui->state.current_point);
}

void ui_move_point(UI* ui, Vec2f point)
{
    ui->state.current_point = normalized(ui, point);
}

UIMode ui_set_mode(UI* ui, UIMode new_mode)
{
    UIMode mode = ui->state.mode;
    ui->state.mode = new_mode;
    return mode;
}

static Vec2f normalized(UI* ui, Vec2f point)
{
    return point / ui_window_size(ui->window);
}

static Vec2f to_screen(UI* ui, Vec2f point)
{
    return point * ui_window_size(ui->window);
}

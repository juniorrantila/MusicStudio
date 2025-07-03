#include "./Context.h"

#include "./Color.h"

#include <Layout/Layout.h>
#include <Core/Time.h>
#include <UI/KeyCode.h>
#include <UI/Window.h>
#include <GL/GL.h>
#include <Layout/Sugar.h>
#include <Core/Print.h>
#include <Clay/Clay.h>

#include "./UI.h"
#include <FS/FSVolume.h>

using namespace LayoutSugar;

static f64 average(f64 entries, f64 current, f64 value);

ErrorOr<Context> context_create(FSVolume* bundle)
{
    auto const* main_typeface = TRY(bundle->open("Fonts/OxaniumLight/Oxanium-Light.ttf"s).or_error(Error::from_string_literal("could not open main typeface")));

    auto* layout = layout_create(nullptr, [](void*, char const* data, usize size) {
        auto message = StringView::from_parts(data, size);
        dprintln("Layout error: {}", message);
    });
    if (!layout) {
        return Error::from_string_literal("could not create layout");
    }

    int main_typeface_id = layout_add_typeface(layout, "main", (u8 const*)main_typeface->items, main_typeface->count);
    if (main_typeface_id < 0) {
        return Error::from_string_literal("could not load main typeface");
    }

    return Context {
        .notes = {},
        .layout = layout,
        .main_typeface = (u16)main_typeface_id,
        .frame = 1,

        .beats_per_minute = 120.0,
        .subdivisions = 16,
        .current_subdivision = 0,
        .tracker = {},
        .is_playing = (u8)0,

        .avg_layout = 0.0,
        .avg_render = 0.0,
        .avg_update = 0.0,
        .delta_time = 0.0,
        .rt = {
            .write = nullptr,
            .underflow_count = 0zu,
            .seconds_offset = 0.0,
            .latency = 0.0,
            .process_time = 0.0,
        },
    };
}

void context_destroy(Context* context)
{
    layout_destroy(context->layout);
}

void context_window_did_resize(Context* context, UIWindow* window)
{
    auto size = ui_window_size(window);
    auto pixel_ratio = ui_window_pixel_ratio(window);
    layout_set_size(context->layout, size, pixel_ratio);
    glViewport(0, 0, (i32)(size.x * pixel_ratio), (i32)(size.y * pixel_ratio));
}

void context_window_did_scroll(Context* context, UIWindow* window)
{
    // FIXME: Scroll behaves strangely
    static f64 last;
    f64 now = Core::time();
    if (now - last < 0.016) {
        return;
    }
    auto scroll = ui_window_scroll_delta(window);
    auto pixel_ratio = ui_window_pixel_ratio(window);
    layout_update_scroll(context->layout, scroll / (pixel_ratio * 2), 0.016f);
    last = now;
}

void context_update(Context* context, UIWindow *window)
{
    auto start = Core::time();
    auto mouse_state = ui_window_mouse_state(window);
    auto mouse_position = ui_window_mouse_pos(window);

    layout_set_pointer_state(context->layout, mouse_position, mouse_state.left_down != 0);

    auto [l, layout_time] = Core::benchmark([&] {
        return context_layout(context);
    });

    auto render_time = Core::benchmark([&] {
        glClearColor(0.5, 0.5, 0.5, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        layout_render(context->layout, l);
    });

    auto end = Core::time();
    context->avg_layout = average(context->frame, context->avg_layout, layout_time);
    context->avg_render = average(context->frame, context->avg_render, render_time);
    context->avg_update = average(context->frame, context->avg_update, end - start);
}


Clay_RenderCommandArray context_layout(Context* context)
{
    Clay_BeginLayout();

    vstack("MainContainer"sv, [=]{
        titlebar(context);
        main_content(context);
    });

    return Clay_EndLayout();
}


void titlebar(Context*)
{
    Element().config([=]{
        id("Titlebar"sv);
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_fixed(28),
            },
            .padding = Clay_Padding{
                .left = 16,
                .right = 16,
                .top = 8,
                .bottom = 8,
            },
            .childGap = {},
            .childAlignment = {},
            .layoutDirection = {},
        });
        rectangle({
            .color = COLOR_BACKGROUND,
            .cornerRadius = {},
        });
    });
}


void main_content(Context* context)
{
    hstack("MainContent"sv, [=]{
        Element().config([]{
            id("Controls"sv);
            layout({
                .sizing = {
                    .width = sizing_fixed(64),
                    .height = sizing_fit(),
                },
                .padding = {},
                .childGap = {},
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER,
                },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            });
        }).body([=]{
            button({
                .id = "play/pause"sv,
                .text = context->is_playing ? "pause"sv : "play"sv,
                .font_id = 0,
                .text_align = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER,
                },
                .on_hover_user = (iptr)context,
                .on_hover = [](Clay_ElementId, Clay_PointerData pointer, iptr data){
                    Context* context = (Context*)data;
                    if (pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                        context->is_playing = !context->is_playing;
                        context->rt.seconds_offset = 0;
                        context->current_subdivision = 0;
                        context->rt.process_time = 0;
                    }
                },
            });
        }),
        // browser(context);
        pinboard(context);
    });
}


void context_set_notes_from_keymap(Context* context, Midi::Note base_note, u8 const* keymap)
{
    u8 base = (u8)base_note;
    if (base >= ((u8)Midi::Note::__Size) - 2 * 12) {
        base = ((u8)Midi::Note::__Size) - 2 * 12 - 1;
    }
    auto* notes = context->notes;
    notes[base + (u8)Midi::Note::C0] = keymap[UIKeyCode_Z];
    notes[base + (u8)Midi::Note::CS0] = keymap[UIKeyCode_S];
    notes[base + (u8)Midi::Note::D0] = keymap[UIKeyCode_X];
    notes[base + (u8)Midi::Note::DS0] = keymap[UIKeyCode_D];
    notes[base + (u8)Midi::Note::E0] = keymap[UIKeyCode_C];
    notes[base + (u8)Midi::Note::F0] = keymap[UIKeyCode_V];
    notes[base + (u8)Midi::Note::FS0] = keymap[UIKeyCode_G];
    notes[base + (u8)Midi::Note::G0] = keymap[UIKeyCode_B];
    notes[base + (u8)Midi::Note::GS0] = keymap[UIKeyCode_H];
    notes[base + (u8)Midi::Note::A0] = keymap[UIKeyCode_N];
    notes[base + (u8)Midi::Note::AS0] = keymap[UIKeyCode_J];
    notes[base + (u8)Midi::Note::B0] = keymap[UIKeyCode_M];

    notes[base + (u8)Midi::Note::C1] = keymap[UIKeyCode_Q];
    notes[base + (u8)Midi::Note::CS1] = keymap[UIKeyCode_2];
    notes[base + (u8)Midi::Note::D1] = keymap[UIKeyCode_W];
    notes[base + (u8)Midi::Note::DS1] = keymap[UIKeyCode_3];
    notes[base + (u8)Midi::Note::E1] = keymap[UIKeyCode_E];
    notes[base + (u8)Midi::Note::F1] = keymap[UIKeyCode_R];
    notes[base + (u8)Midi::Note::FS1] = keymap[UIKeyCode_5];
    notes[base + (u8)Midi::Note::G1] = keymap[UIKeyCode_T];
    notes[base + (u8)Midi::Note::GS1] = keymap[UIKeyCode_6];
    notes[base + (u8)Midi::Note::A1] = keymap[UIKeyCode_Y];
    notes[base + (u8)Midi::Note::AS1] = keymap[UIKeyCode_7];
    notes[base + (u8)Midi::Note::B1] = keymap[UIKeyCode_U];
}

static f64 average(f64 entries, f64 current, f64 value)
{
    return ((entries * current) + value) / (entries + 1.0);
}

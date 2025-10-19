#include "./Context.h"

#include "./Color.h"

#include <LibUI/KeyCode.h>
#include <LibLayout/Layout.h>
#include <LibLayout/Sugar.h>
#include <LibCore/Print.h>

using namespace LayoutSugar;

enum class VerticalNotes {
    No = false,
    Yes = true,
};

static void white_note(ID note_name, u8 _Atomic* note, VerticalNotes vertical);
static void black_note(ID note_name, u8 _Atomic* note, VerticalNotes vertical);

static void white_block(u8 _Atomic* note, u8 playing, VerticalNotes vertical);
static void black_block(u8 _Atomic* note, u8 playing, VerticalNotes vertical);

static void piano_octave(MSContext* context, u8 start_note, VerticalNotes vertical_notes);
static void block_octave(MSContext* context, u8 start_note, u32 beat, u32 subdivision, VerticalNotes vertical_notes);
static void note_on_hover(Clay_ElementId element, Clay_PointerData data, iptr user);
static void block_on_hover(Clay_ElementId element, Clay_PointerData data, iptr user);

static auto white_note_width = 24.0f;
static auto white_note_height = 64.0f;

static auto black_note_width = 24.0f;
static auto black_note_height = 64.0f;

void piano_roll_vertical(MSContext* context)
{
    Element().config([=]{
        id("PianoRoll"sv);
        CLAY_SCROLL({
            .horizontal = false,
            .vertical = true,
        });
        layout({
            .sizing = {
                .width = sizing_fit(),
                .height = sizing_fit(),
            },
            .padding = {},
            .childGap = 1,
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        });
    }).body([=]{
        piano_octave(context, (u8)Midi::Note::C6, VerticalNotes::No);
        piano_octave(context, (u8)Midi::Note::C5, VerticalNotes::No);
        piano_octave(context, (u8)Midi::Note::C4, VerticalNotes::No);
        piano_octave(context, (u8)Midi::Note::C3, VerticalNotes::No);
    });
}


void piano_roll_horizontal(MSContext* context)
{
    Element().config([=]{
        id("PianoRoll"sv);
        CLAY_SCROLL({
            .horizontal = true,
            .vertical = true,
        });
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = {},
            .childGap = 1,
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_BOTTOM,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
    }).body([=]{
        piano_octave(context, (u8)Midi::Note::C0, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C1, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C2, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C3, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C4, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C5, VerticalNotes::Yes);
        piano_octave(context, (u8)Midi::Note::C6, VerticalNotes::Yes);
    });
}


static void piano_octave(MSContext* context, u8 start_note, VerticalNotes vertical_notes)
{
    u32 note_index = 0;
    u8 _Atomic* note = nullptr;

    note_index = start_note + (u32)Midi::Note::C0;
    note = &context->notes[note_index];
    white_note({ "C", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::CS0;
    note = &context->notes[note_index];
    black_note({ "C#", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::D0;
    note = &context->notes[note_index];
    white_note({ "D", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::DS0;
    note = &context->notes[note_index];
    black_note({ "D#", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::E0;
    note = &context->notes[note_index];
    white_note({ "E", note_index }, note, vertical_notes);

    note_index = start_note + (u32)Midi::Note::F0;
    note = &context->notes[note_index];
    white_note({ "F", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::FS0;
    note = &context->notes[note_index];
    black_note({ "F#", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::G0;
    note = &context->notes[note_index];
    white_note({ "G", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::GS0;
    note = &context->notes[note_index];
    black_note({ "G#", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::A0;
    note = &context->notes[note_index];
    white_note({ "A", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::AS0;
    note = &context->notes[note_index];
    black_note({ "A#", note_index }, note, vertical_notes);
    
    note_index = start_note + (u32)Midi::Note::B0;
    note = &context->notes[note_index];
    white_note({ "B", note_index }, note, vertical_notes);
}


static void block_octave(MSContext* context, u8 start_note, u32 beat, u32 subdivision, VerticalNotes vertical_notes)
{
    u32 note_index = 0;
    u8 _Atomic* note = nullptr;
    u32 current_division = context->current_subdivision;
    u8 playing = 0;
    bool is_current_subdivision = context->is_playing && (current_division == (beat * context->subdivisions + subdivision));
    playing = is_current_subdivision;

    note_index = start_note + (u32)Midi::Note::B0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::AS0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    black_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::A0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::GS0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    black_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::G0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::FS0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    black_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::F0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::E0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::DS0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    black_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::D0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);

    note_index = start_note + (u32)Midi::Note::CS0;
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    black_block(note, playing, vertical_notes);

    note_index = (start_note + (u32)Midi::Note::C0);
    note = &context->tracker[beat][subdivision][note_index];
    // playing = *note && is_current_subdivision;
    white_block(note, playing, vertical_notes);
}


static void white_note(ID note_name, u8 _Atomic* note, VerticalNotes vertical)
{
    auto width = sizing_fixed(white_note_width);
    auto height = sizing_fixed(white_note_height);
    box(note_name, [&]{
        return LayoutSugar::BoxConfig{
            .corner_radius = {
                .topLeft = 0,
                .topRight = 0,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
            .padding = {
                .left = (u16)(vertical == VerticalNotes::Yes ? 0 : 4),
                .right = 0,
                .top = 0,
                .bottom = (u16)(vertical == VerticalNotes::Yes ? 4 : 0),
            },
            .sizing = {
                .width = vertical == VerticalNotes::Yes ? width : height,
                .height = vertical == VerticalNotes::Yes ? height : width,
            },
            .color = *note ? COLOR_ORANGE : Clay_Hovered() ? COLOR_BLUE : COLOR_WHITE,
            .child_alignment = {
                .x = vertical == VerticalNotes::Yes ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT,
                .y = vertical == VerticalNotes::Yes ? CLAY_ALIGN_Y_BOTTOM : CLAY_ALIGN_Y_CENTER,
            },
            .on_hover = note_on_hover,
            .on_hover_user = (iptr)note,
        };
    }, [=]{
        text(note_name.text, {
            .textColor = COLOR_BLACK,
            .fontId = 0,
            .fontSize = 12,
            .letterSpacing = {},
            .lineHeight = {},
            .wrapMode = {},
        });
    });
}


static void black_note(ID note_name, u8 _Atomic* note, VerticalNotes vertical)
{
    auto width = sizing_fixed(black_note_width);
    auto height = sizing_fixed(black_note_height);
    box(note_name, [&]{
        return LayoutSugar::BoxConfig{
            .corner_radius = {
                .topLeft = 0,
                .topRight = 0,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
            .padding = {
                .left = (u16)(vertical == VerticalNotes::Yes ? 0 : 2),
                .right = 0,
                .top = 0,
                .bottom = (u16)(vertical == VerticalNotes::Yes ? 2 : 0),
            },
            .sizing = {
                .width = vertical == VerticalNotes::Yes ? width : height,
                .height = vertical == VerticalNotes::Yes ? height : width,
            },
            .color = *note ? COLOR_ORANGE : Clay_Hovered() ? COLOR_BLUE : COLOR_BLACK,
            .child_alignment = {
                .x = vertical == VerticalNotes::Yes ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT,
                .y = vertical == VerticalNotes::Yes ? CLAY_ALIGN_Y_BOTTOM : CLAY_ALIGN_Y_CENTER,
            },
            .on_hover = note_on_hover,
            .on_hover_user = (iptr)note,
        };
    }, [=]{
        text(note_name.text, {
            .textColor = COLOR_WHITE,
            .fontId = 0,
            .fontSize = 12,
            .letterSpacing = {},
            .lineHeight = {},
            .wrapMode = {},
        });
    });
}


static void white_block(u8 _Atomic* note, u8 playing, VerticalNotes vertical)
{
    auto width = sizing_fixed(white_note_width);
    auto height = sizing_fixed(white_note_width);
    box({}, [&]{
        return LayoutSugar::BoxConfig{
            .corner_radius = { 2, 2, 2, 2 },
            .padding = {
                .left = (u16)(vertical == VerticalNotes::Yes ? 0 : 4),
                .right = 0,
                .top = 0,
                .bottom = (u16)(vertical == VerticalNotes::Yes ? 4 : 0),
            },
            .sizing = {
                .width = vertical == VerticalNotes::Yes ? width : height,
                .height = vertical == VerticalNotes::Yes ? height : width,
            },
            .color = playing ? COLOR_BLUE : *note ? COLOR_ORANGE : Clay_Hovered() ? COLOR_BLUE : COLOR_BACKGROUND,
            .child_alignment = {
                .x = vertical == VerticalNotes::Yes ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT,
                .y = vertical == VerticalNotes::Yes ? CLAY_ALIGN_Y_BOTTOM : CLAY_ALIGN_Y_CENTER,
            },
            .on_hover = block_on_hover,
            .on_hover_user = (iptr)note,
        };
    }, [&]{
    });
}


static void black_block(u8 _Atomic* note, u8 playing, VerticalNotes vertical)
{
    auto width = sizing_fixed(black_note_width);
    auto height = sizing_fixed(white_note_width);
    box({}, [&]{
        return LayoutSugar::BoxConfig{
            .corner_radius = { 2, 2, 2, 2 },
            .padding = {
                .left = (u16)(vertical == VerticalNotes::Yes ? 0 : 2),
                .right = 0,
                .top = 0,
                .bottom = (u16)(vertical == VerticalNotes::Yes ? 2 : 0),
            },
            .sizing = {
                .width = vertical == VerticalNotes::Yes ? width : height,
                .height = vertical == VerticalNotes::Yes ? height : width,
            },
            .color = playing ? COLOR_BLUE : *note ? COLOR_ORANGE : Clay_Hovered() ? COLOR_BLUE : COLOR_BACKGROUND,
            .child_alignment = {
                .x = vertical == VerticalNotes::Yes ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT,
                .y = vertical == VerticalNotes::Yes ? CLAY_ALIGN_Y_BOTTOM : CLAY_ALIGN_Y_CENTER,
            },
            .on_hover = block_on_hover,
            .on_hover_user = (iptr)note,
        };
    }, [&]{
    });
}


void piano_tracker(MSContext* context)
{
    Element().config([=]{
        id("PianoTracker"sv);
        CLAY_SCROLL({
            .horizontal = true,
            .vertical = true,
        });
        layout({
            .sizing = {
                .width = sizing_fit(),
                .height = sizing_fit(),
            },
            .padding = {},
            .childGap = 1,
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
        rectangle({
            .color = COLOR_BORDER,
            .cornerRadius = {},
        });
    }).body([=]{
        for (u32 beat = 0; beat < context->beats_per_minute; beat++) {
            for (u32 subdivision = 0; subdivision < context->subdivisions; subdivision++) {
                u32 index = beat * context->beats_per_minute + subdivision;
                Element().config([=]{
                    id({"Subbeat"sv, index});
                    layout({
                        .sizing = {
                            .width = sizing_fit(),
                            .height = sizing_fit(),
                        },
                        .padding = {},
                        .childGap = 1,
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_LEFT,
                            .y = CLAY_ALIGN_Y_TOP,
                        },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    });
                }).body([=]{
                    block_octave(context, (u8)Midi::Note::C6, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C5, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C4, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C3, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C2, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C1, beat, subdivision, VerticalNotes::No);
                    block_octave(context, (u8)Midi::Note::C0, beat, subdivision, VerticalNotes::No);
                });
            }
        }
    });
}


static void note_on_hover(Clay_ElementId element, Clay_PointerData data, iptr user)
{
    (void)element;
    u8 _Atomic* key = (u8 _Atomic*)user;
    if (data.state == CLAY_POINTER_DATA_PRESSED) {
        *key = true;
    }
    if (data.state == CLAY_POINTER_DATA_RELEASED) {
        *key = false;
    }
}

static void block_on_hover(Clay_ElementId element, Clay_PointerData data, iptr user)
{
    (void)element;
    u8 _Atomic* key = (u8 _Atomic*)user;
    static u8 toggle = 0;
    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        toggle = !*key;
    }
    if (data.state == CLAY_POINTER_DATA_PRESSED) {
        *key = toggle;
    }
}

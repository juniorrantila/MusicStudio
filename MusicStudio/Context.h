#pragma once
#include <UI/Forward.h>
#include <Ty/Base.h>
#include <Midi/Note.h>
#include <Layout/Layout.h>
#include <Ty/ErrorOr.h>
#include <FS/FSVolume.h>

struct Context {
    u8 _Atomic notes[(u8)Midi::Note::__Size];

    Layout* layout;
    u16 main_typeface;
    u64 frame;

    f64 beats_per_minute;
    u32 subdivisions;
    u32 current_subdivision;
    u8 _Atomic tracker[120][16][(u8)Midi::Note::__Size];
    u8 _Atomic is_playing;

    f64 avg_layout;
    f64 avg_render;
    f64 avg_update;
    f64 delta_time;

    // Updated by real-time thread.
    struct {
        void (*write)(void*, f64);
        usize _Atomic underflow_count;
        f64 _Atomic seconds_offset;
        f64 _Atomic latency;
        f64 _Atomic process_time;
    } rt;
};

ErrorOr<Context> context_create(FSVolume* bundle);
void context_destroy(Context* context);

void context_set_notes_from_keymap(Context*, Midi::Note base_note, u8 const* keymap);

Clay_RenderCommandArray context_layout(Context* context);

void context_window_did_resize(Context* context, UIWindow* window);
void context_update(Context* context, UIWindow* window);
void context_window_did_scroll(Context* context, UIWindow* window);

void titlebar(Context*);
void main_content(Context*);
void browser(Context*);
void piano_roll_vertical(Context*);
void piano_roll_horizontal(Context*);
void piano_tracker(Context*);
void pinboard(Context*);

#pragma once
#include <LibUI/Forward.h>
#include <LibTy/Base.h>
#include <LibMIDI/Note.h>
#include <LibLayout/Layout.h>
#include <LibTy/ErrorOr.h>
#include <LibCore/FSVolume.h>

struct MSContext {
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

ErrorOr<MSContext> context_create(FSVolume* bundle);
void context_destroy(MSContext* context);

void context_set_notes_from_keymap(MSContext*, Midi::Note base_note, u8 const* keymap);

Clay_RenderCommandArray context_layout(MSContext* context);

void context_window_did_resize(MSContext* context, UIWindow* window);
void context_update(MSContext* context, UIWindow* window);
void context_window_did_scroll(MSContext* context, UIWindow* window);

void titlebar(MSContext*);
void main_content(MSContext*);
void browser(MSContext*);
void piano_roll_vertical(MSContext*);
void piano_roll_horizontal(MSContext*);
void piano_tracker(MSContext*);
void pinboard(MSContext*);

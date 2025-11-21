#pragma once
#include <Basic/Types.h>

#include "./UI/UI.h"
#include "./Audio/Audio.h"

#include <Basic/Allocator.h>
#include <Basic/Arena.h>
#include <Basic/Bits.h>
#include <Basic/Context.h>
#include <Basic/DeferredFileLogger.h>
#include <Basic/FileLogger.h>
#include <Basic/Mailbox.h>
#include <Basic/MemoryPoker.h>

#include <LibAudio/AudioManager.h>
#include <LibCore/Actor.h>
#include <LibCore/FSVolume.h>
#include <LibGL/Renderer.h>
#include <LibLayout2/Layout.h>
#include <LibUI/Window.h>

#include <SoundIo/SoundIo.h>

#define MAGIC(s) 0 \
    | ((u64)s[7]) << (7 * 8) \
    | ((u64)s[6]) << (6 * 8) \
    | ((u64)s[5]) << (5 * 8) \
    | ((u64)s[4]) << (4 * 8) \
    | ((u64)s[3]) << (3 * 8) \
    | ((u64)s[2]) << (2 * 8) \
    | ((u64)s[1]) << (1 * 8) \
    | ((u64)s[0]) << (0 * 8)
typedef enum : u64 {
    Magic_Settings = MAGIC("settings"),
    Magic_Playback = MAGIC("playback"),
    Magic_MS_State = MAGIC("ms state"),
    Magic_Sections = MAGIC("sections"),
} Magic;
#undef MAGIC

typedef struct PlayAudioEvent {
    AUAudio audio;
    f64 start_pulse;
} PlayAudioEvent;

typedef struct {
    bool use_auto_reload : 1;
    bool use_audio : 1;
    bool use_ui : 1;
} StateFlags;

typedef enum SystemID : u8 {
    SystemID_Main,
    SystemID_Actor,
    SystemID_IO,
    SystemID_PriorityIO,
    SystemID_Layout,
    SystemID_Render,
    SystemID_Audio,
    SystemID__Count,
} SystemID;

typedef struct PersistedSettings {
    Magic magic; // magic_settings
    u64 version; // sizeof(*this)

    f64 max_latency;
    f64 frames_per_second; // a.k.a "sample rate"
    f64 quarter_notes_per_second; // a.k.a "tempo"
    f64 pulses_per_quarter_note; // a.k.a "ppqn"
} PersistedSettings;

typedef struct PersistedPlayback {
    Magic magic; // magic_playback
    u64 version; // sizeof(*this)

    f64 current_pulse;
    f64 current_pulse_offset;
    u64 underflow_count;
    f64 last_sample;
} PersistedPlayback;

typedef struct StableMain {
    u64 version; // sizeof(*this)

    StateFlags flags;

    MemoryPoker memory_poker;

    Mailbox mailbox_grid[SystemID__Count][SystemID__Count]; // [receiver][sender]
} StableMain;

typedef struct StableActorReloader {
    u64 version; // sizeof(*this)

    FSVolume volume;

    Actor io;
    Actor layout;
    Actor render;
    Actor audio;
} StableActorReloader;

typedef struct StableIO {
    u64 version; // sizeof(*this)

    struct IOActor const* actor;

    FileLogger log;
    FSVolume volume;
} StableIO;

typedef struct StablePriorityIO {
    u64 version; // sizeof(*this)

    struct IOActor const* actor;

    FileLogger log;
    FSVolume volume;
} StablePriorityIO;

typedef struct StableLayout {
    u64 version; // sizeof(*this)

    UIActor const* actor;

    FileLogger log;

    Layout layout;
} StableLayout;

typedef struct StableRender {
    u64 version; // sizeof(*this)

    struct RenderActor const* actor;

    FileLogger log;
    FSVolume volume;

    GLRenderer render;
} StableRender;

typedef struct StableAudio {
    u64 version; // sizeof(*this)

    AudioActor const* actor;

    DeferredFileLogger log;

    SoundIo* soundio;
    SoundIoOutStream* outstream;

    AUAudioManager audio_manager;

    f64 channel_buffer[au_audio_channel_max][4096];
} StableAudio;

typedef struct StableState {
    u64 version; // sizeof(*this)

    StableMain main;
    StableActorReloader actor_reloader;
    StableIO io;
    StablePriorityIO priority_io;
    StableLayout layout;
    StableRender render;
    StableAudio audio;
} StableState;

typedef union TransMain {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
    };
} TransMain;
static_assert(sizeof(TransMain) == 64 * KiB);
static_assert(alignof(TransMain) >= 8);

typedef union TransActorReloader {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
    };
} TransActorReloader;
static_assert(sizeof(TransActorReloader) == 64 * KiB);
static_assert(alignof(TransActorReloader) >= 8);

typedef union TransIO {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
        u8 arena_buffer[8 * KiB];
    };
} TransIO;
static_assert(sizeof(TransIO) == 64 * KiB);
static_assert(alignof(TransIO) >= 8);

typedef union TransPriorityIO {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
        u8 arena_buffer[8 * KiB];
    };
} TransPriorityIO;
static_assert(sizeof(TransPriorityIO) == 64 * KiB);
static_assert(alignof(TransPriorityIO) >= 8);

typedef union TransLayout {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
    };
} TransLayout;
static_assert(sizeof(TransLayout) == 64 * KiB);
static_assert(alignof(TransLayout) >= 8);

typedef union TransRender {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)

        v2 resolution;
        f32 z_max;
    };
} TransRender;
static_assert(sizeof(TransRender) == 64 * KiB);
static_assert(alignof(TransRender) >= 8);

typedef union TransAudio {
    u8 buffer[64 * KiB];
    struct {
        u64 version; // sizeof(*this)
    };
} TransAudio;
static_assert(sizeof(TransAudio) == 64 * KiB);
static_assert(alignof(TransAudio) >= 8);

typedef struct TransState {
    u64 version; // sizeof(*this)

    TransMain main;
    TransActorReloader actor_reloader;
    TransIO io;
    TransPriorityIO priority_io;
    TransLayout layout;
    TransRender render;
    TransAudio audio;
} TransState;

typedef struct PersistedState {
    Magic magic; // magic_ms_state
    u64 version; // sizeof(*this)

    struct {
        Magic magic; // magic_sections
        u64 version; // sizeof(*this)

        PersistedSettings* settings;
        PersistedPlayback* playback;
    } sections;

    // NOTE: These should be accessed via the pointers in sections.
    struct {
        PersistedSettings settings;
        PersistedPlayback playback;
    } unsafe;
} PersistedState;

typedef struct State {
    u64 version; // sizeof(*this)

    PersistedState persisted;
    StableState stable;
    TransState trans;
} State;

C_API void state_init(State*, StateFlags);

C_API void actor_reloader_frame(StableActorReloader*, TransActorReloader*);
C_API void io_frame(StableIO*, TransIO*);
C_API void priority_io_frame(StablePriorityIO*, TransPriorityIO*);
C_API void layout_frame(StableLayout* stable, TransLayout* trans, UIWindow* window, Mailbox* render_sink);
C_API void render_frame(StableRender*, TransRender*, Mailbox* sink);

C_API [[nodiscard]] bool main_start(State*);
C_API [[nodiscard]] bool actor_reloader_start(State*);
C_API [[nodiscard]] bool io_start(State*);
C_API [[nodiscard]] bool priority_io_start(State*);
C_API [[nodiscard]] bool audio_start(State*);

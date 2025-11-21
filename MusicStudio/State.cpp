#include "./State.h"

#include "./UI/UI.h"
#include "./Audio/Audio.h"

#include <Basic/Base.h>
#include <Basic/Bits.h>
#include <Basic/Context.h>
#include <Basic/Defer.h>
#include <Basic/Mailbox.h>
#include <Basic/PageAllocator.h>
#include <Basic/Verify.h>

#include <LibAudio/AudioManager.h>
#include <LibCore/FSVolume.h>
#include <LibCore/Time.h>
#include <LibLayout2/Layout.h>
#include <LibLayout2/RenderCommand.h>
#include <LibTy/StringView.h>

#include <Shaders/Shaders.h>

#include <stdlib.h>
#include <math.h>
#include <pthread.h>

static void persisted_init(PersistedState*);
static void persisted_settings_init(PersistedSettings*);
static void persisted_playback_init(PersistedPlayback*);

static void stable_init(StableState*, StateFlags);
static void stable_main_init(StableState*, StateFlags);
static void stable_actor_reloader_init(StableActorReloader*, StateFlags);
static void stable_io_init(StableIO*, StateFlags);
static void stable_priority_io_init(StablePriorityIO*, StateFlags);
static void stable_layout_init(StableState*, StateFlags);
static void stable_render_init(StableState*, StateFlags);
static void stable_audio_init(StableState*, StateFlags);

static void trans_init(TransState*);

static SoundIoOutStream* create_default_outstream(State*, SoundIo*);
static void* io_loop(void* user);
static void* priority_io_loop(void* user);

static void audio_frame(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) noexcept [[clang::nonblocking]];

C_API void state_init(State* state, StateFlags flags)
{
    VERIFY(!ty_is_initialized(state));
    defer [=] { ty_set_initialized(state); };

    persisted_init(&state->persisted);
    VERIFY(ty_is_initialized(&state->persisted));

    stable_init(&state->stable, flags);
    VERIFY(ty_is_initialized(&state->stable));

    trans_init(&state->trans);
    VERIFY(ty_is_initialized(&state->trans));
}

static SoundIoOutStream* create_default_outstream(State* state, SoundIo* soundio)
{
    VERIFY(ty_is_initialized(&state->persisted.sections));
    VERIFY(ty_is_initialized(state->persisted.sections.playback));
    VERIFY(ty_is_initialized(&state->stable.audio));

    int index = soundio_default_output_device_index(soundio);
    if (index < 0) {
        state->stable.audio.log->error("could not get default output device index");
        return nullptr;
    }
    auto* device = soundio_get_output_device(soundio, index);
    device->software_latency_max = state->persisted.sections.settings->max_latency;
    device->software_latency_min = 0;
    auto* outstream = soundio_outstream_create(device);
    if (!outstream) {
        state->stable.audio.log->error("could not create outstream");
        return nullptr;
    }
    outstream->userdata = state;
    outstream->format = SoundIoFormatFloat64NE;
    outstream->write_callback = audio_frame;
    outstream->sample_rate = (i32)state->persisted.sections.settings->frames_per_second;
    outstream->error_callback = [](SoundIoOutStream* stream, int err){
        auto* host = (State*)stream->userdata;
        host->stable.audio.log->error("soundio: %s", soundio_strerror(err));
    };
    outstream->underflow_callback = [](SoundIoOutStream* stream){
        auto* state = (State*)stream->userdata;
        state->stable.audio.log->warning("underflow %zu", ++state->persisted.sections.playback->underflow_count);
    };
    return outstream;
}

static void* io_loop(void* user)
{
    init_default_context("io");

    State* ctx = (State*)user;
    auto* io = &ctx->stable.io;
    auto* trans = &ctx->trans.io;
    auto* mailbox_grid = &ctx->stable.main.mailbox_grid;
    auto* mailboxes = &(*mailbox_grid)[SystemID_IO];
    auto arena = fixed_arena_init(&trans->arena_buffer, ARRAY_SIZE(trans->arena_buffer));
    auto* volume = &io->volume;
    volume->automount_when_not_found = true;

    u64 mailbox_count = ARRAY_SIZE(*mailboxes);
    for (u64 i = 0; i < mailbox_count; i++) {
        (*mailboxes)[i].reader();
    }

    for (;;) {
        reset_temporary_arena();
        arena.drain();

        mailbox_wait_any();

        for (u64 i = 0; i < mailbox_count; i++) {
            auto* mailbox = &(*mailboxes)[i];
            u16 tag = 0;
            while (mailbox->reader()->peek(&tag).found) {
                arena.drain();

                switch (tag) {
                case Ty2::type_id<DeferredLogEvent>(): {
                    DeferredLogEvent event;
                    if (verify(mailbox->reader()->read(&event).ok).failed) continue;
                    event.logger->handle_event(&event);
                    continue;
                }
                default:
                    io->log->warning("unknown message kind %s, ignoring it", ty_type_name(tag));
                    mailbox->reader()->toss(tag);
                }
            }
        }
    }
    UNREACHABLE();
    return nullptr;
}

static void* priority_io_loop(void* user)
{
    init_default_context("priority-io");

    State* ctx = (State*)user;
    auto* io = &ctx->stable.priority_io;
    auto* trans = &ctx->trans.priority_io;
    auto* mailbox_grid = &ctx->stable.main.mailbox_grid;
    auto* mailboxes = &(*mailbox_grid)[SystemID_PriorityIO];
    auto arena = fixed_arena_init(&trans->arena_buffer, ARRAY_SIZE(trans->arena_buffer));
    auto* volume = &io->volume;
    volume->automount_when_not_found = true;

    u64 mailbox_count = ARRAY_SIZE(*mailboxes);
    for (u64 i = 0; i < mailbox_count; i++) {
        (*mailboxes)[i].reader();
    }

    for (;;) {
        reset_temporary_arena();
        arena.drain();

        mailbox_wait_any();

        for (u64 i = 0; i < mailbox_count; i++) {
            auto* mailbox = &(*mailboxes)[i];
            u16 tag = 0;
            while (mailbox->reader()->peek(&tag).found) {
                arena.drain();

                switch (tag) {
                case Ty2::type_id<DeferredLogEvent>(): {
                    DeferredLogEvent event;
                    if (verify(mailbox->reader()->read(&event).ok).failed) continue;
                    event.logger->handle_event(&event);
                    continue;
                }
                default:
                    io->log->warning("unknown message kind %s, ignoring it", ty_type_name(tag));
                    mailbox->reader()->toss(tag);
                }
            }
        }
    }
    UNREACHABLE();
    return nullptr;
}

static void audio_frame(SoundIoOutStream* outstream, int, int frame_count_max) noexcept [[clang::nonblocking]]
{
    pthread_setname_np("audio");

    SoundIoChannelArea* areas = nullptr;
    auto* ctx = (State*)outstream->userdata;
    VERIFY(ctx->stable.main.flags.use_audio);

    auto* stable = &ctx->stable.audio;
    auto* trans = &ctx->trans.audio;
    auto* persisted = &ctx->persisted;
    auto const* actor = stable->actor;
    u8 arena_buffer[8 * KiB];
    FixedArena arena = fixed_arena_init(arena_buffer, sizeof(arena_buffer));

    Context context = (Context){
        .log = &stable->log.logger,
        .temp_arena = &arena,
    };
    set_context(&context);

    int frames_left = frame_count_max;
    for (;;) {
        int frame_count = frames_left;
        if (auto err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
            stable->log->fatal("unrecoverable stream error: %s", soundio_strerror(err));
        }

        if (!frame_count)
            break;

        VERIFY(((u32)frame_count_max) <= ARRAY_SIZE(stable->channel_buffer[0]));
        f64* channels[au_audio_channel_max] = {};
        auto* layout = &outstream->layout;
        for (i32 i = 0; i < layout->channel_count; i++)
            channels[i] = stable->channel_buffer[0];
        memzero(stable->channel_buffer, layout->channel_count * sizeof(stable->channel_buffer[0]));

        actor->audio_frame(persisted, stable, trans, channels, frame_count_max, layout->channel_count);
        for (int channel = 0; channel < layout->channel_count; channel += 1) {
            for (int frame = 0; frame < frame_count_max; frame += 1) {
                static_assert(sizeof(channels[channel][frame]) == 8);
                *((f64*)areas[channel].ptr) = channels[channel][frame];
                areas[channel].ptr += areas[channel].step;
            }
        }

        if (auto err = soundio_outstream_end_write(outstream)) {
            if (err == SoundIoErrorUnderflow)
                return;
            stable->log->fatal("unrecoverable stream error: %s", soundio_strerror(err));
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }
}

C_API void layout_frame(StableLayout* stable, TransLayout* trans, UIWindow* window, Mailbox* render_sink)
{
    VERIFY(ty_is_initialized(stable));
    ty_trans_migrate(trans);

    auto size = ui_window_size(window);
    auto ratio = ui_window_pixel_ratio(window);
    auto mouse = ui_window_mouse_pos(window);
    auto mouse_state = ui_window_mouse_state(window);

    layout_begin(&stable->layout, (LayoutInputState){
        .render_sink = render_sink,
        .current_time = (f32)core_time_since_unspecified_epoch(),
        .frame_bounds_x = size.x,
        .frame_bounds_y = size.y,
        .pixel_ratio = ratio,
        .mouse_x = mouse.x,
        .mouse_y = mouse.y,
        .mouse_left_down = mouse_state.left_down,
        .mouse_right_down = mouse_state.right_down,

        .scroll_delta_x = 0, // FIXME
        .scroll_delta_y = 0, // FIXME
    });
    stable->actor->layout_frame(stable, trans);
    layout_end(&stable->layout);
}

C_API void render_frame(StableRender* stable, TransRender* trans, Mailbox* sink)
{
    VERIFY(ty_is_initialized(stable));
    ty_trans_migrate(trans);

    auto shader = [&](GLShaderSource path) -> GLShaderID
    {
        FileID vert_file;
        if (!fs_volume_find(&stable->volume, path.vert, &vert_file))
            return gl_shader_id_null;
        FileID frag_file;
        if (!fs_volume_find(&stable->volume, path.frag, &frag_file))
            return gl_shader_id_null;
        auto shader = stable->render.shader((GLShaderSource){
            .vert = fs_content(fs_volume_use(&stable->volume, vert_file)),
            .frag = fs_content(fs_volume_use(&stable->volume, frag_file)),
        });
        return shader;
    };

    auto* render = &stable->render;
    auto* log = &stable->log.logger;

    struct timespec time {};
    auto events = fs_volume_poll_events(&stable->volume, &time);
    for (u32 i = 0; i < events.count; i++) {
        switch (events.kind[i]) {
        case FSEventKind_Modify:
        case FSEventKind_Create:
            fs_file_reload(fs_volume_use_ref(&stable->volume, events.file[i]));
            break;
        case FSEventKind_Delete:
            break;
        }
    }

    render->uniform1f(render->uniform("time"), (f32)core_time_since_unspecified_epoch());
    render->clear((v4){0.4, 0.2, 0.2, 1});

    f32 z_index = 0;
    u16 tag = 0;

    while (sink->reader()->peek(&tag).found) {
        switch (tag) {
        case Ty2::type_id<LayoutRenderSetResolution>(): {
            LayoutRenderSetResolution command;
            if (!sink->reader()->read(&command).ok) {
                log->error("could not unwrap resolution update command");
                continue;
            }
            trans->resolution.x = command.width;
            trans->resolution.y = command.height;
            render->uniform2f(render->uniform("resolution"), trans->resolution);
            continue;
        }

        case Ty2::type_id<LayoutRenderFlush>(): {
            sink->reader()->toss(tag);
            render->flush();
            trans->z_max = z_index;
            z_index = 0;
            continue;
        }

        case Ty2::type_id<LayoutRenderRectangle>(): {
            LayoutRenderRectangle command;
            if (!sink->reader()->read(&command).ok) {
                log->error("could not unwrap resolution update command");
                continue;
            }

            z_index += 1;

            v2 p = (v2){command.bounding_box.point.x, command.bounding_box.point.y} / trans->resolution; // 0..1
            p *= (v2){2, -2}; // X and Y are range 0..2 and Y is flipped.
            p += (v2){-1, 1}; // X and y are range -1..1

            v2 s = ((v2){command.bounding_box.size.width, command.bounding_box.size.height} / trans->resolution);
            s *= (v2){2, -2};

            v4 color = (v4){command.color.r, command.color.g, command.color.b, command.color.a};
            render->push_quad(shader((GLShaderSource){
                .vert = "Shaders/simple.vert"s,
                .frag = "Shaders/simple.frag"s,
            }), (GLQuad){
                {
                    .color      = color,
                    .bits       = { z_index, trans->z_max, 0, 0 },
                    .position   = { p.x, p.y },
                    .uv0        = { 0, 0 },
                    .uv1        = { 0, 0 },
                },
                {
                    .color      = color,
                    .bits       = { z_index, trans->z_max, 0, 0 },
                    .position   = { p.x + s.x, p.y },
                    .uv0        = { 0, 0 },
                    .uv1        = { 0, 0 },
                },
                {
                    .color      = color,
                    .bits       = { z_index, trans->z_max, 0, 0 },
                    .position   = { p.x, p.y + s.y },
                    .uv0        = { 0, 0 },
                    .uv1        = { 0, 0 },
                },
                {
                    .color      = color,
                    .bits       = { z_index, trans->z_max, 0, 0 },
                    .position   = { p.x + s.x, p.y + s.y },
                    .uv0        = { 0, 0 },
                    .uv1        = { 0, 0 },
                },
            });
            continue;
        }

        default:
            log->error("unknown render command: %s, ignoring it", ty_type_name(tag));
            sink->reader()->toss(tag);
            continue;
        }
    }
}

static void actor_reloader_loop(State* state)
{
    StateFlags flags = state->stable.main.flags;
    VERIFY(flags.use_auto_reload);
    auto* actor = &state->stable.actor_reloader;
    for (;;) {
        reset_temporary_arena();

        fs_volume_poll_events(&actor->volume, nullptr);
        if (flags.use_audio) actor_update(&actor->audio);
        if (flags.use_ui) actor_update(&actor->layout);

        if (flags.use_audio && actor_needs_reload(&actor->audio)) {
            if (actor_reload(&actor->audio)) infof("reloaded audio");
            else warnf("could not reload audio");
        }

        if (flags.use_ui && actor_needs_reload(&actor->layout)) {
            if (actor_reload(&actor->layout)) infof("reloaded layout");
            else warnf("could not reload layout");
        }
    }
}

static void persisted_init(PersistedState* state)
{
    VERIFY(!ty_is_initialized(state));
    memzero(state);
    defer [=] { ty_set_initialized(state); };

    state->magic = Magic_MS_State;

    auto* sections = &state->sections;
    VERIFY(!ty_is_initialized(sections));
    memzero(sections);
    defer [=] { ty_set_initialized(sections); };

    sections->settings = &state->unsafe.settings;
    persisted_settings_init(sections->settings);

    sections->playback = &state->unsafe.playback;
    persisted_playback_init(sections->playback);
}

static void persisted_settings_init(PersistedSettings* settings)
{
    *settings = (PersistedSettings){
        .magic = Magic_Settings,
        .version = sizeof(*settings),
        .max_latency = 0.001, // 1ms
        .frames_per_second = 44100,
        .quarter_notes_per_second = 120,
        .pulses_per_quarter_note = 960,
    };
}

static void persisted_playback_init(PersistedPlayback* playback)
{
    *playback = (PersistedPlayback){
        .magic = Magic_Playback,
        .version = sizeof(*playback),
        .current_pulse = 0,
        .current_pulse_offset = 0,
        .underflow_count = 0,
        .last_sample = 0.0,
    };
}

static void stable_init(StableState* stable, StateFlags flags)
{
    VERIFY(!ty_is_initialized(stable));
    memzero(stable);
    defer [=] { ty_set_initialized(stable); };

    stable_main_init(stable, flags);
    stable_actor_reloader_init(&stable->actor_reloader, flags);
    stable_io_init(&stable->io, flags);
    stable_priority_io_init(&stable->priority_io, flags);
    stable_layout_init(stable, flags);
    stable_render_init(stable, flags);
    stable_audio_init(stable, flags);
}

static void stable_main_init(StableState* state, StateFlags flags)
{
    auto* main = &state->main;
    VERIFY(!ty_is_initialized(main));
    memzero(main);
    defer [=] { ty_set_initialized(main); };

    main->flags = flags;
    memory_poker_init(&main->memory_poker);
    for (u32 receiver = 0; receiver < SystemID__Count; receiver++) {
        if (receiver == SystemID_Audio && !flags.use_audio)
            continue;
        if (receiver == SystemID_Layout && !flags.use_ui)
            continue;
        if (receiver == SystemID_Render && !flags.use_ui)
            continue;
        for (u32 sender = 0; sender < SystemID__Count; sender++) {
            if (sender == SystemID_Audio && !flags.use_audio)
                continue;
            if (sender == SystemID_Layout && !flags.use_ui)
                continue;
            if (sender == SystemID_Render && !flags.use_ui)
                continue;

            auto* mailbox = &main->mailbox_grid[receiver][sender];

            if (receiver == SystemID_Audio || sender == SystemID_Audio) {
                if (!mailbox_init(64 * KiB, mailbox).ok) {
                    fatalf("could not initialize mailboxes");
                }
                mailbox->attach_memory_poker(&main->memory_poker);
                continue;
            }
            if (!mailbox_lazy_init(64 * KiB, mailbox).ok) {
                fatalf("could not initialize mailboxes");
            }
        }
    }
}

static void stable_actor_reloader_init(StableActorReloader* actor, StateFlags flags)
{
    VERIFY(!ty_is_initialized(actor));
    memzero(actor);
    defer [=] { ty_set_initialized(actor); };

    fs_volume_init(&actor->volume);
    actor->volume.automount_when_not_found = flags.use_auto_reload;

    if (flags.use_ui) {
        if (!ui_actor_init(&actor->layout, &actor->volume, flags.use_auto_reload))
            fatalf("could not create layout actor");
    }

    if (flags.use_audio) {
        if (!audio_actor_init(&actor->audio, &actor->volume, flags.use_auto_reload))
            fatalf("could not create audio actor");
    }
}

static void stable_io_init(StableIO* io, StateFlags)
{
    VERIFY(!ty_is_initialized(io));
    memzero(io);
    defer [=] { ty_set_initialized(io); };

    c_string name = "io";
    io->log = file_logger_init(name, stderr);

    fs_volume_init(&io->volume);
    io->volume.automount_when_not_found = true;
}

static void stable_priority_io_init(StablePriorityIO* io, StateFlags)
{
    VERIFY(!ty_is_initialized(io));
    memzero(io);
    defer [=] { ty_set_initialized(io); };

    c_string name = "priority-io";
    io->log = file_logger_init(name, stderr);

    fs_volume_init(&io->volume);
    io->volume.automount_when_not_found = true;
}

static void stable_layout_init(StableState* stable, StateFlags flags)
{
    if (!flags.use_ui) return;
    VERIFY(ty_is_initialized(&stable->actor_reloader));
    auto* layout = &stable->layout;
    VERIFY(!ty_is_initialized(layout));
    memzero(layout);
    defer [=] { ty_set_initialized(layout); };

    layout->actor = (UIActor const*)&stable->actor_reloader.layout.dispatch;
    VERIFY(layout->actor != nullptr);

    layout->log = file_logger_init("layout", stderr);
    layout_init(&layout->layout, &layout->log.logger);
}

static void stable_render_init(StableState* stable, StateFlags flags)
{
    if (!flags.use_ui) return;
    auto* render = &stable->render;
    VERIFY(!ty_is_initialized(render));
    memzero(render);
    defer [=] { ty_set_initialized(render); };

    render->log = file_logger_init("render", stderr);

    fs_volume_init(&render->volume);
    UseBakedShaders use_baked = flags.use_auto_reload ? UseBakedShaders_Yes : UseBakedShaders_No;
    if (!Shaders::add_to_volume(&render->volume, use_baked))
        render->log->fatal("could not add shaders to volume");
    render->volume.automount_when_not_found = flags.use_auto_reload;
}

static void stable_audio_init(StableState* stable, StateFlags flags)
{
    if (!flags.use_audio) return;

    VERIFY(ty_is_initialized(&stable->main));
    VERIFY(ty_is_initialized(&stable->actor_reloader));

    auto* audio = &stable->audio;
    VERIFY(!ty_is_initialized(audio));
    memzero(audio);
    defer [=] { ty_set_initialized(audio); };

    stable->main.memory_poker.push(&stable->audio, sizeof(stable->audio));

    VERIFY(ty_is_initialized(&stable->io));
    audio->log = deferred_file_logger_init("audio", &stable->main.mailbox_grid[SystemID_IO][SystemID_Audio], stderr);

    if (!au_audio_manager_init(&audio->audio_manager, &stable->main.memory_poker))
        fatalf("could not initialize audio manager");

    audio->actor = (AudioActor const*)&stable->actor_reloader.audio.dispatch;
    VERIFY(audio->actor != nullptr);
}

static void trans_init(TransState* trans)
{
    VERIFY(!ty_is_initialized(trans));
    memzero(trans);
    defer [=] { ty_set_initialized(trans); };
}


C_API [[nodiscard]] bool main_start(State* state)
{
    VERIFY(ty_is_initialized(&state->stable.main));

    if (!state->stable.main.memory_poker.start())
        errorf("could not start main memory poker");
    return true;
}

C_API [[nodiscard]] bool actor_reloader_start(State* state)
{
    VERIFY(ty_is_initialized(&state->stable.main));
    VERIFY(ty_is_initialized(&state->stable.actor_reloader));
    VERIFY(state->stable.main.flags.use_auto_reload);
    infof("starting actor-reloader loop");

    pthread_t thread;
    if (pthread_create(&thread, nullptr, [](void* user) -> void* {
        init_default_context("actor-reloader");
        actor_reloader_loop((State*)user);
        UNREACHABLE();
        return nullptr;
    }, state) != 0)
        return false;
    pthread_detach(thread);
    return true;
}

C_API [[nodiscard]] bool io_start(State* state)
{
    pthread_t io_thread;
    int res = pthread_create(&io_thread, nullptr, io_loop, state);
    if (res < 0) {
        return false;
    }
    pthread_detach(io_thread);
    return true;
}

C_API [[nodiscard]] bool priority_io_start(State* state)
{
    pthread_t io_thread;
    int res = pthread_create(&io_thread, nullptr, priority_io_loop, state);
    if (res < 0) {
        return false;
    }
    pthread_detach(io_thread);
    return true;
}

C_API [[nodiscard]] bool audio_start(State* state)
{
    auto* stable = &state->stable;
    VERIFY(ty_is_initialized(&stable->main));
    VERIFY(state->stable.main.flags.use_audio);
    VERIFY(ty_is_initialized(&stable->audio));
    VERIFY(ty_is_initialized(&stable->actor_reloader));

    auto* audio = &state->stable.audio;

    if (!au_audio_manager_start(&audio->audio_manager)) {
        errorf("could not start audio manager");
        return false;
    }

    audio->soundio = soundio_create();
    if (!audio->soundio)
        fatalf("could not create soundio");
    if (auto error = soundio_connect(audio->soundio))
        fatalf("could not connect soundio: %s", soundio_strerror(error));

    soundio_flush_events(audio->soundio);
    audio->outstream = create_default_outstream(state, audio->soundio);
    if (!audio->outstream) fatalf("could not create default outstream");

    infof("starting audio loop");
    if (soundio_outstream_open(stable->audio.outstream) != 0) {
        errorf("could not open audio outstream");
        return false;
    }
    if (soundio_outstream_start(stable->audio.outstream) != 0) {
        errorf("could not start audio outstream");
        return false;
    }
    return true;
}

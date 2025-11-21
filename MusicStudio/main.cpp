#include <LibMain/Main.h>

#include <Basic/Array.h>
#include <Basic/Context.h>
#include <Basic/PageAllocator.h>
#include <Basic/Verify.h>
#include <Basic/ByteDecoder.h>
#include <Basic/SharedBuffer.h>

#include <LibCLI/ArgumentParser.h>
#include <LibCore/Time.h>
#include <LibTy/Try.h>
#include <LibUI/Application.h>
#include <LibCore/Core.h>
#include <Basic/Defer.h>

#include "./State.h"

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    init_default_context("main");

    auto argument_parser = CLI::ArgumentParser();

    u32 app_hints = 0;
    TRY(argument_parser.add_flag("--native-like", "-nl", "run native-like mode", [&]{
        app_hints |= UIApplicationHint_NativeLike;
    }));

    StateFlags flags = (StateFlags){
        .use_auto_reload = true,
        .use_audio = false,
        .use_ui = false,
    };

    TRY(argument_parser.add_flag("--no-actor-reload", "-nar", "disable reload of actors", [&]{
        flags.use_auto_reload = false;
    }));

    TRY(argument_parser.add_flag("--with-audio", "-a", "enable audio", [&]{
        flags.use_audio = true;
    }));

    TRY(argument_parser.add_flag("--with-ui", "-u", "enable ui", [&]{
        flags.use_ui = true;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    State* state = (State*)page_alloc(sizeof(*state));
    static_assert(sizeof(*state) < 96 * MiB);
    VERIFY(state != nullptr);
    state_init(state, flags);
    if (!flags.use_audio && !flags.use_ui)
        return 0;
    if (!io_start(state))
        fatalf("could not start io");
    if (!priority_io_start(state))
        fatalf("could not start priority io");
    if (!main_start(state))
        fatalf("could not start main");

    UIApplication* app = nullptr;
    UIWindow* window = nullptr;
    if (flags.use_ui) {
        auto* stable = &state->stable;
        VERIFY(ty_is_initialized(&stable->layout));
        VERIFY(ty_is_initialized(&stable->render));

        app = ui_application_create(app_hints);
        if (!app) fatalf("could not create application");
        window = ui_window_create(app, {
            .parent = nullptr,
            .title = "GUIShowcase",
            .x = 0,
            .y = 0,
            .width = 800,
            .height = 600,
        });
        ui_window_autosave(window, "ms.music-studio.gui-showcase");

        ui_window_set_resize_callback(window, state, [](UIWindow* window, void* user) {
            auto* state = (State*)user;
            auto* stable = &state->stable;
            auto* trans = &state->trans;

            VERIFY(ty_is_initialized(&stable->layout));
            VERIFY(ty_is_initialized(&stable->render));
            auto* sink = &stable->main.mailbox_grid[SystemID_Render][SystemID_Layout];
            layout_frame(&stable->layout, &trans->layout, window, sink);
            ui_window_gl_make_current_context(window);
            render_frame(&stable->render, &trans->render, sink);
            ui_window_gl_flush(window);
        });

        ui_window_show(window);
    }

    bool audio_only = !flags.use_ui && flags.use_audio;

    if (flags.use_audio) {
        if (!audio_start(state))
            fatalf("could not start audio");
    }
    if (flags.use_auto_reload) {
        if (!actor_reloader_start(state))
            errorf("could not start actor reloader");
    }

    infof("starting main loop");
    for (;;) {
        reset_temporary_arena();

        if (flags.use_ui) {
            ui_application_poll_events(app);
            if (ui_window_should_close(window))
                break;
        }

        if (flags.use_audio)
            soundio_flush_events(state->stable.audio.soundio);

        if (audio_only) {
            soundio_wait_events(state->stable.audio.soundio);
            continue;
        }

        if (flags.use_ui) {
            auto* stable = &state->stable;
            auto* trans = &state->trans;

            auto* sink = &state->stable.main.mailbox_grid[SystemID_Render][SystemID_Layout];
            layout_frame(&stable->layout, &trans->layout, window, sink);
            ui_window_gl_make_current_context(window);
            render_frame(&stable->render, &trans->render, sink);
            ui_window_gl_flush(window);
        }

    }

    return {};
}

#pragma once
#include "./Forward.h"

#include "./FileBrowser.h"
#include "./Toolbar.h"
#include "./EventLoop.h"

#include <FS/Bundle.h>
#include <MS/Plugin.h>
#include <Rexim/LA.h>
#include <Ty/Signal.h>
#include <UI/Application.h>
#include <UI/UI.h>

struct Application {
    static constexpr auto titlebar_size  = 32.0f;

    static ErrorOr<Application> create(FS::Bundle& bundle);

    void run();

    ErrorOr<void> open_plugin(StringView path);
    ErrorOr<void> change_path(StringView path);

private:
    void handle_events();
    void render(Vec4f box);

    Application(UI::Application&&, UI::UI&&, FileBrowser&&);

    UI::Application m_application;
    UI::UI m_ui;
    FileBrowser m_file_browser;
    EventLoop m_event_loop {};
    Toolbar m_toolbar {};
    Vector<MS::Plugin> m_plugins {};

    Signal<StringBuffer> m_current_path {};
};

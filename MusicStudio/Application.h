#pragma once
#include "./Forward.h"

#include "./FileBrowser.h"
#include "./DeviceBrowser.h"
#include "./EventLoop.h"
#include "UIView/Forward.h"
#include "UIView/View.h"

#include <FS/Bundle.h>
#include <MS/Plugin.h>
#include <Rexim/LA.h>
#include <Ty/Signal.h>
#include <UI/Application.h>
#include <UI/UI.h>
#include <SoundIo/SoundIo.h>
#include <UIView/FrameCapture.h>

struct Application {
    static constexpr auto titlebar_size  = 32.0f;

    static ErrorOr<Application> create(FS::Bundle&, SoundIo*);

    void run();

    ErrorOr<void> open_plugin(StringView path);
    ErrorOr<void> change_path(StringView path);

    ErrorOr<void> connect_default_backend();

    UIView::ViewBase* view();
    operator UIView::ViewBase*() { return view(); }

private:
    Application(UI::Application&&, UI::UI&&, FileBrowser&&, DeviceBrowser&&, SoundIo* soundio);

    SoundIo* m_soundio { nullptr };
    UI::Application m_application;
    UI::UI m_ui;
    FileBrowser m_file_browser;
    DeviceBrowser m_device_browser;
    EventLoop m_event_loop {};
    Vector<MS::Plugin> m_plugins {};

    Signal<StringBuffer> m_current_path {};
};

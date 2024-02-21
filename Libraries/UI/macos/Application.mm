#import <AppKit/AppKit.h>
#import "./UIApp.h"

#include "../Application.h"
#include "../Window.h"

namespace UI {

ErrorOr<Application> Application::create(StringView title, i32 x, i32 y, i32 width, i32 height)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    UIApp* app = [[UIApp alloc]
        initWithContentRect:NSMakeRect(x, y, width, height)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:YES];
    app.title = [NSString stringWithFormat:@"%.*s", title.size(), title.data()];
    [NSApp setDelegate:app];

    return Application(app, width, height);
}

Application::Application(NativeHandle native_handle, f32 width, f32 height)
    : m_native_handle(native_handle)
    , m_width(width)
    , m_height(height)
{
    UIApp* app = native_handle;
    app.instance = this;
}

Application::~Application()
{
    if (is_valid()) {
        invalidate();
    }
}

void Application::handle_move(Application* into)
{
    UIApp* app = m_native_handle;
    app.instance = into;
}

void Application::update() const
{
    if (on_update) {
        on_update();
    }
}

void Application::run() const {
    UIApp* app = m_native_handle;
    [app makeKeyAndOrderFront:app];
    [NSApp run];
}

void Application::add_child_window(Window const& window) const
{
    UIApp* app = m_native_handle;
    NSView* view = window.native_handle();
    [app addChildWindow:view.window ordered:NSWindowAbove];
}

}

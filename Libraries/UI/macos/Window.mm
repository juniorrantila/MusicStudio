#include "../Window.h"
#include "./Rexim/LA.h"

#include <Ty/Try.h>

#import <AppKit/AppKit.h>

namespace UI {

struct MacOSWindowHandle : WindowHandle {
    __strong NSWindow* window;
    __strong NSView* view;

    ~MacOSWindowHandle() override = default;
    void* native_handle() const override
    {
        CFBridgingRetain(window);
        return (void*)CFBridgingRetain(view);
    }

    void* native_window() const override
    {
        return (void*)CFBridgingRetain(window);
    }

    void resize(i32 x, i32 y) override
    {
        auto window_size = vec2f(window.frame.size.width, window.frame.size.height);
        auto new_size = vec2f(x, y + 32.0f);
        auto size_delta = new_size - window_size;

        auto frame = window.frame;
        frame.origin.x += -size_delta.x;
        frame.origin.y += size_delta.y;
        frame.size.width = new_size.x;
        frame.size.height = new_size.y;
        [window setFrame:frame display:YES];
        [view setFrameOrigin:NSMakePoint(0, 0)];
    }
};

ErrorOr<RefPtr<Window>> Window::create(StringView title, i32 x, i32 y, i32 width, i32 height)
{
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(x, y, width, height)
                                                   styleMask:NSWindowStyleMaskClosable
                                                            |NSWindowStyleMaskTitled
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.title = [NSString stringWithFormat:@"%.*s", title.size(), title.data()];

    auto* handle = new MacOSWindowHandle();
    handle->window = window;
    handle->view = window.contentView;

    return TRY(RefPtr<Window>::create(handle));
}

}

#include "../Window.h"
#import <AppKit/AppKit.h>

namespace UI {

Window::Window(NativeHandle handle)
    : m_handle(handle)
{
}

ErrorOr<Window> Window::create(StringView title, i32 x, i32 y, i32 width, i32 height)
{
    auto* window = [[NSWindow alloc] initWithContentRect: NSMakeRect(x, y, width, height)
                                            styleMask: NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                              backing: NSBackingStoreBuffered
                                                defer: YES];
    window.title = [NSString stringWithFormat:@"%.*s", title.size(), title.data()]; 
    return Window(window);
}

Window::~Window()
{
}

id Window::native_handle() const
{
    NSWindow* window = m_handle;
    return [window contentView];
}

void Window::show() const
{
    NSWindow* window = (NSWindow*)m_handle;
    [window makeKeyAndOrderFront:window];
}

void Window::run() const
{
    if (![NSApp isRunning]) {
        [NSApp run];
    }
}

}

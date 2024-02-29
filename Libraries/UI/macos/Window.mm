#include "../Window.h"
#import <AppKit/AppKit.h>

namespace UI {

Window::Window(void const* handle)
    : m_handle((void*)handle)
{
}

ErrorOr<Window> Window::create(StringView title, i32 x, i32 y, i32 width, i32 height)
{
    auto* window = [[NSWindow alloc] initWithContentRect: NSMakeRect(x, y, width, height)
                                            styleMask: NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                              backing: NSBackingStoreBuffered
                                                defer: YES];
    window.title = [NSString stringWithFormat:@"%.*s", title.size(), title.data()]; 
    auto* handle = CFBridgingRetain(window);
    return Window(handle);
}

Window::~Window()
{
    if (m_handle != nullptr) {
        CFRelease(m_handle);
    }
}

void* Window::native_handle() const
{
    NSWindow* window = (__bridge NSWindow*)m_handle;
    return (void*)CFBridgingRetain([window contentView]);
}

void Window::show() const
{
    NSWindow* window = (__bridge NSWindow*)m_handle;
    [window makeKeyAndOrderFront:window];
}

void Window::run() const
{
    if (![NSApp isRunning]) {
        [NSApp run];
    }
}

}

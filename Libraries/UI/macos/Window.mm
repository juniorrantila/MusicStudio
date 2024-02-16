#include "../Window.h"

#import <Cocoa/Cocoa.h>

namespace UI {

ErrorOr<Window> Window::create(StringView title, i32 x, i32 y, i32 width, i32 height)
{
    [NSApplication sharedApplication];
    auto* window = [[NSWindow alloc] initWithContentRect: NSMakeRect(x, y, width, height)
                                            styleMask: NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                              backing: NSBackingStoreBuffered
                                                defer: YES];
    window.title = [NSString stringWithFormat:@"%.*s", title.size(), title.data()]; 
    [window retain];
    return Window(window);
}

Window::~Window()
{
    [(NSWindow*)m_handle release];
}

void* Window::native_handle() const
{
    return [(NSWindow*)m_handle contentView];
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

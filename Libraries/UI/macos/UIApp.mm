#import "./UIApp.h"
#include "Application.h"
#include <Graphics/GL.h>
#include <AppKit/AppKit.h>

@implementation UIApp

@synthesize glView;

BOOL shouldStop = NO;

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
    if (self = [super initWithContentRect:contentRect
                                styleMask:aStyle
                                  backing:bufferingType
                                    defer:flag]) {
        [self setTitle:[[NSProcessInfo processInfo] processName]];

        // clang-format off
        NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersion3_2Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAccelerated,
            NSOpenGLPFANoRecovery,
            0
        };
        // clang-format on

        NSOpenGLPixelFormat* format =
            [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
        glView = [[NSOpenGLView alloc] initWithFrame:contentRect pixelFormat:format];
        // Set context and attach it to the window
        [[glView openGLContext] makeCurrentContext];
        [glView setWantsBestResolutionOpenGLSurface:YES];

        [self setContentView:glView];
        [glView prepareOpenGL];
        [self makeKeyAndOrderFront:self];
        [self setAcceptsMouseMovedEvents:YES];
        [self makeKeyWindow];
        [self setOpaque:YES];

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    return self;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
    glViewport(0, 0, frameSize.width, frameSize.height);
    if (self.instance) {
        if (self.instance->on_window_resize) {
            self.instance->on_window_resize(frameSize.width, frameSize.height);
        }
        self.instance->set_width(frameSize.width);
        self.instance->set_height(frameSize.height);
    }
    return frameSize;
}

- (void)keyDown:(NSEvent *)event {
    if (self.instance && self.instance->on_key_down) {
        self.instance->on_key_down((UI::KeyCode)event.keyCode, event.modifierFlags);
    }
}

- (void)keyUp:(NSEvent *)event {
    if (self.instance && self.instance->on_key_up) {
        self.instance->on_key_up((UI::KeyCode)event.keyCode, event.modifierFlags);
    }
}

- (void)mouseDown:(NSEvent *)event {
    if (self.instance && self.instance->on_mouse_down) {
        self.instance->on_mouse_down();
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (self.instance && self.instance->on_mouse_up) {
        self.instance->on_mouse_up();
    }
}

- (void)mouseMoved:(NSEvent *)event {
    if (self.instance && self.instance->on_mouse_move) {
        auto loc = event.locationInWindow;
        self.instance->on_mouse_move(loc.x, loc.y);
    }
}

- (void)scrollWheel:(NSEvent *)event {
    if (self.instance && self.instance->on_scroll) {
        self.instance->on_scroll(event.scrollingDeltaX, event.scrollingDeltaY);
    }
}

- (void)update:(NSTimer*)timer
{
    if (shouldStop) {
        [self close];
        return;
    }
    if ([self isVisible]) {
        if (self.instance) {
            self.instance->update();
        }
        [glView update];
        [[glView openGLContext] flushBuffer];
    }
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                     target:self
                                   selector:@selector(update:)
                                   userInfo:nil
                                    repeats:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)aNotification
{
    shouldStop = YES;
}

@end

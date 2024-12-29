#include "../Window.h"

#include "../KeyCode.h"

#include <GL/GL.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

@interface UIAppKitWindow : NSWindow<NSWindowDelegate> {
    @public u8 keymap[UI_KEYMAP_SIZE];
    @public UIMouseState mouse_state;
    @public bool should_close;
    @public NSSize size;
    @public bool is_fullscreen;

    @public void* resize_user;
    @public void(*resize_callback)(UIWindow*, void* user);
    @public UIApplication* application;
}
@property (nonatomic, retain) NSOpenGLView* glView;

- (instancetype)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag;

@end

static UIAppKitWindow* create_window(UIWindowSpec spec)
{
    auto* window = [[UIAppKitWindow alloc]
        initWithContentRect:NSMakeRect(spec.x, spec.y, spec.width, spec.height)
                  styleMask:0
                           | NSWindowStyleMaskFullSizeContentView
                           | NSWindowStyleMaskUnifiedTitleAndToolbar
                           | NSWindowStyleMaskTitled
                           | NSWindowStyleMaskClosable
                           | NSWindowStyleMaskMiniaturizable
                           | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:YES];
    window.titlebarAppearsTransparent = true;
    window.delegate = window;
    if (spec.title) {
        window.title = [NSString stringWithUTF8String:spec.title];
    }
    window->size = NSMakeSize(spec.width, spec.height);
    return window;
}

UIWindow* ui_window_create(UIApplication* app, UIWindowSpec spec)
{
    auto* parent = (__bridge UIAppKitWindow*)spec.parent;
    auto* window = create_window(spec);
    window->application = app;
    if (parent) {
        [parent addChildWindow:window ordered:NSWindowAbove];
    } else {
        [window makeKeyAndOrderFront:nil];
    }
    return (UIWindow*)CFBridgingRetain(window);
}

UIApplication* ui_window_application(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return window->application;
}

void ui_window_destroy(UIWindow* win)
{
    ui_window_close(win);
}

void* ui_window_native_handle(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return (__bridge NSView*)window.contentView;
}

void ui_window_close(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    [window close];
}

bool ui_window_should_close(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return window->should_close;
}

int ui_window_show(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    [[window contentView] setHidden:NO];
    return 0;
}

Vec2f ui_window_size(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return vec2f(window->size.width, window->size.height);
}

f32 ui_window_pixel_ratio(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    NSRect contentRect = window.glView.frame;
    NSRect fbRect = [window.glView convertRectToBacking:contentRect];
    return fbRect.size.width / contentRect.size.width;
}

u8 const* ui_window_keymap(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return window->keymap;
}

Vec2f ui_window_mouse_pos(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    Vec2f window_size = ui_window_size(win);

    auto mousePosition = [window convertPointFromScreen:[NSEvent mouseLocation]];
    return vec2f(mousePosition.x, window_size.y - mousePosition.y);
}

UIMouseState ui_window_mouse_state(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    return window->mouse_state;
}

c_string ui_window_strerror(int error)
{
    if (error != 0) return "unknown error";
    return "no error";
}

void ui_window_gl_make_current_context(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    [window.glView.openGLContext makeCurrentContext];
}

void ui_window_gl_flush(UIWindow* win)
{
    auto* window = (__bridge UIAppKitWindow*)win;
    [window.glView.openGLContext flushBuffer];
}

int ui_window_set_resize_callback(UIWindow* win, void* user, void(*callback)(UIWindow* window, void* user))
{
    auto* window = (__bridge UIAppKitWindow*)win;
    window->resize_user = user;
    window->resize_callback = callback;
    return 0;
}

bool ui_window_is_fullscreen(UIWindow const* window)
{
    auto* win = (__bridge UIAppKitWindow const*)window;
    return win->is_fullscreen;
}

@implementation UIAppKitWindow

- (instancetype)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)style
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
    self = [super initWithContentRect:contentRect
                                styleMask:style
                                  backing:bufferingType
                                    defer:flag];
    if (self == nil) return nil;

    [self setTitle:[[NSProcessInfo processInfo] processName]];

    // clang-format off
    NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAColorFloat,
        NSOpenGLPFAColorSize, 64,
        NSOpenGLPFAAlphaSize, 16,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        0
    };
    // clang-format on

    NSOpenGLPixelFormat* format =
        [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
    _glView = [[NSOpenGLView alloc] initWithFrame:contentRect pixelFormat:format];
    // Set context and attach it to the window
    [[_glView openGLContext] makeCurrentContext];
    [_glView setWantsBestResolutionOpenGLSurface:YES];
    [_glView setWantsExtendedDynamicRangeOpenGLSurface:YES];

    [self setContentView:_glView];
    [_glView prepareOpenGL];
    [self setAcceptsMouseMovedEvents:YES];
    [self setOpaque:YES];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return self;
}

-(void)flagsChanged:(NSEvent *)event
{
    auto flags = event.modifierFlags;
    self->keymap[UIKeyCode_COMMAND] = (flags & NSEventModifierFlagCommand) > 0;
    self->keymap[UIKeyCode_CONTROL] = (flags & NSEventModifierFlagControl) > 0;
    self->keymap[UIKeyCode_OPTION] = (flags & NSEventModifierFlagOption) > 0;
    self->keymap[UIKeyCode_SHIFT] = (flags & NSEventModifierFlagShift) > 0;
}

-(void)keyDown:(NSEvent *)event
{
    auto keyCode = event.keyCode;
    assert(keyCode < sizeof(self->keymap) / sizeof(self->keymap[0]));
    self->keymap[keyCode] = true;
}

-(void)keyUp:(NSEvent *)event
{
    auto keyCode = event.keyCode;
    assert(keyCode < sizeof(self->keymap) / sizeof(self->keymap[0]));
    self->keymap[keyCode] = false;
}

-(void)mouseDown:(NSEvent *)event
{
    self->mouse_state.left_down = event.clickCount;
}

-(void)mouseUp:(NSEvent *)event
{
    self->mouse_state.left_down = false;
}

-(NSSize)windowWillResize:(NSWindow *)window toSize:(NSSize)proposedFrameSize
{
    self->size = proposedFrameSize;
    if (self->resize_callback) {
        self->resize_callback((UIWindow*)self, self->resize_user);
    }
    return proposedFrameSize;
}

-(BOOL)windowShouldClose:(NSNotification *)notification
{
    self->should_close = true;
    return true;
}

-(void)windowWillEnterFullScreen:(NSNotification *)notification
{
    self->is_fullscreen = true;
}

-(void)windowWillExitFullScreen:(NSNotification *)notification
{
    self->is_fullscreen = false;
}

@end

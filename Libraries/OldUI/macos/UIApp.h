#pragma once
#import <AppKit/AppKit.h>
#include "../Application.h"
#include "../Graphics/GL.h"

@interface UIApp : NSWindow <NSApplicationDelegate, NSWindowDelegate> {
}
@property (atomic) UI::Application* instance;
@property (nonatomic, retain) NSOpenGLView* glView;

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag;

- (void)update:(NSTimer*)timer;

@end

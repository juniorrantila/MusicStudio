#include "../Application.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

@interface UIApplicationDelegate : NSObject<NSApplicationDelegate>
+(UIApplicationDelegate*) sharedInstance;
@end


UIApplication* ui_application_create(u32 hints)
{
    auto* application = [NSApplication sharedApplication];
    if (hints & UIApplicationHint_NativeLike) {
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
    application.delegate = UIApplicationDelegate.sharedInstance;
    auto* app = (UIApplication*)CFBridgingRetain(application);
    [application run];
    return app;
}

void ui_application_poll_events(UIApplication* application)
{
    auto* app = (__bridge NSApplication*)application;
    @autoreleasepool {
        for (;;) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event == nil)
                break;

            [app sendEvent:event];
        }
    }
}

void ui_application_destroy(UIApplication* app)
{
    auto* application = (__bridge NSApplication*)app;
    [application terminate:nil];
    CFBridgingRelease(app);
}

UICursor ui_application_cursor(UIApplication* app)
{
    (void)app;
    NSCursor* cursor = NSCursor.currentCursor;
    if (cursor == NSCursor.arrowCursor) {
        return UICursor_Arrow;
    }
    if (cursor == NSCursor.pointingHandCursor) {
        return UICursor_Pointer;
    }
    if (cursor == NSCursor.closedHandCursor) {
        return UICursor_ClosedHand;
    }
    assert(false && "Unknown cursor");
}

void ui_application_cursor_push(UIApplication* app, UICursor cursor)
{
    (void)app;
    switch (cursor) {
    case UICursor_Arrow:
        [NSCursor.arrowCursor push];
        break;
    case UICursor_Pointer:
        [NSCursor.pointingHandCursor push];
        break;
    case UICursor_ClosedHand:
        [NSCursor.closedHandCursor push];
        break;
    }
}

void ui_application_cursor_set(UIApplication* app, UICursor cursor)
{
    (void)app;
    switch (cursor) {
    case UICursor_Arrow:
        [NSCursor.arrowCursor set];
        break;
    case UICursor_Pointer:
        [NSCursor.pointingHandCursor set];
        break;
    case UICursor_ClosedHand:
        [NSCursor.closedHandCursor set];
        break;
    }
}

void ui_application_cursor_pop(UIApplication* app)
{
    (void)app;
    [NSCursor pop];
}


@implementation UIApplicationDelegate
static UIApplicationDelegate* sharedInstance = nil;

+(UIApplicationDelegate*) sharedInstance
{
    if (sharedInstance == nil) {
        sharedInstance = [[UIApplicationDelegate alloc] init];
    }
    return sharedInstance;
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    return YES;
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    auto* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                     location:NSMakePoint(0, 0)
                                modifierFlags:0
                                    timestamp:0
                                 windowNumber:0
                                      context:nil
                                      subtype:0
                                        data1:0
                                        data2:0];
    [NSApp postEvent:event atStart:YES];
    [NSApp stop:nil];
}

@end

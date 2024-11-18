#include "../Application.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

@interface UIApplicationDelegate : NSObject<NSApplicationDelegate>
@end

UIApplication* ui_application_create(u32 hints)
{
    auto* application = [NSApplication sharedApplication];
    if (hints & UIApplicationHint_NativeLike) {
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    }
    application.delegate = [[UIApplicationDelegate alloc] init];
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
    CFBridgingRelease(application);
}

@implementation UIApplicationDelegate

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

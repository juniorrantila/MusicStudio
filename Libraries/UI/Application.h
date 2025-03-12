#pragma once
#include "./Forward.h"
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UICursor {
    UICursor_Arrow,
    UICursor_Pointer,
    UICursor_ClosedHand,
} UICursor;

enum UIApplicationHint {
    UIApplicationHint_NativeLike = 1 << 0,
};

UIApplication* ui_application_create(u32 hints);
void ui_application_destroy(UIApplication*);
void ui_application_poll_events(UIApplication*);

UICursor ui_application_cursor(UIApplication*);
void ui_application_cursor_push(UIApplication*, UICursor);
void ui_application_cursor_pop(UIApplication*);
void ui_application_cursor_set(UIApplication*, UICursor);

#ifdef __cplusplus
}
#endif

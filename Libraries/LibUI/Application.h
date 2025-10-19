#pragma once
#include "./Forward.h"

#include <Basic/Types.h>

typedef enum UICursor {
    UICursor_Arrow,
    UICursor_Pointer,
    UICursor_ClosedHand,
} UICursor;

enum UIApplicationHint {
    UIApplicationHint_NativeLike = 1 << 0,
};

C_API UIApplication* ui_application_create(u32 hints);
C_API void ui_application_destroy(UIApplication*);
C_API void ui_application_poll_events(UIApplication*);

C_API UICursor ui_application_cursor(UIApplication*);
C_API void ui_application_cursor_push(UIApplication*, UICursor);
C_API void ui_application_cursor_pop(UIApplication*);
C_API void ui_application_cursor_set(UIApplication*, UICursor);

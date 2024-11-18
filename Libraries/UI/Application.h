#pragma once
#include "./Forward.h"
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

enum UIApplicationHint {
    UIApplicationHint_NativeLike = 1 << 0,
};

UIApplication* ui_application_create(u32 hints);
void ui_application_destroy(UIApplication*);
void ui_application_poll_events(UIApplication*);

#ifdef __cplusplus
}
#endif

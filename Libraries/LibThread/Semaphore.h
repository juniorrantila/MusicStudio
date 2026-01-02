#pragma once
#include <Basic/Forward.h>

#include <Basic/Types.h>
#include <Basic/Platform.h>

#if TARGET_OS_OSX
#include <mach/task.h>
#include <mach/semaphore.h>
#else
#error "unsupported OS"
#endif

typedef struct THSemaphore THSemaphore;

#if TARGET_OS_OSX
typedef struct THSemaphore {
    task_t owner;
    semaphore_t event;
} THSemaphore;
#endif

C_API void th_sem_init(THSemaphore*, i32 initial_value);
C_API void th_sem_deinit(THSemaphore*);

C_API void th_sem_signal(THSemaphore*);
C_API KError th_sem_wait_for(THSemaphore*, i32 milliseconds);
C_API KError th_sem_wait(THSemaphore*);

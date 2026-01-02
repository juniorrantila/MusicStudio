#pragma once
#include "./Forward.h"

#include <Basic/Types.h>
#include <Basic/Error.h>
#include <Basic/Context.h>
#include <Basic/Platform.h>

#include "./Semaphore.h"

#if PLATFORM_POSIX
#include <pthread.h>
#else
#error "unsupported platform"
#endif

typedef struct THThread THThread;
typedef struct THThread {
    c_string name;
    KThreadID id;
    void* user;
    Context starting_context;
    void (*proc)(void*);

#if PLATFORM_POSIX
    pthread_t thread_handle;
    THSemaphore suspended;
#else
#error "unsupported platform"
#endif
} THThread;

C_API KError th_thread_init(THThread*, c_string name, Context starting_context, void* data, void(*)(void*));
C_API void th_thread_start(THThread*);

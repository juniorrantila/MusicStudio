#pragma once
#include "./Context.h"

#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AsyncFiber AsyncFiber;
typedef struct Async {
    AsyncContext const* master;
    AsyncFiber* fiber;
    void* user;
} Async;

typedef struct AsyncFiber {
    AsyncContext context;
    int state;

    void* stack;
    void* callback;
    Async async;
} AsyncFiber;

AsyncFiber async_create(void* stack, usize stack_size, void* user, void(*callback)(Async, void* user));
void async_continue(AsyncFiber* fiber, AsyncContext const* master);
void async_yield(Async);

#ifdef __cplusplus
}
#endif

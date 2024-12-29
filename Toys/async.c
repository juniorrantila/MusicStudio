#include <Ty/Base.h>
#include <Async/Context.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    AsyncContext context;
    int done;
} AsyncFiber;

typedef struct {
    AsyncContext const* master;
    AsyncFiber* fiber;
} AsyncFiberContext;

void fiber_create(AsyncFiber* fiber, AsyncContext const* master, void* user, void(*callback)(AsyncFiberContext context, void* user));
void fiber_yield(AsyncFiberContext context);
int fiber_is_done(AsyncFiber const*);
void fiber_continue(AsyncFiber const*);

static void loop2(AsyncFiberContext context, void* user)
{
    char const* id = (char const*)user;
    for (int i = 0; i < 3; i++) {
        printf("%s: %d\n", id, i);
        fiber_yield(context);
    }
    printf("Done %s\n", id);
}

int main(void) {
    AsyncContext master = {};
    AsyncFiber fibers[3] = {};
    volatile int counter = 0;

    fiber_create(&fibers[0], &master, (void*)"Foo", loop2);
    fiber_create(&fibers[1], &master, (void*)"Bar", loop2);
    fiber_create(&fibers[2], &master, (void*)"Baz", loop2);

    if (!async_context_save(&master)) {
        counter = ((counter + 1) % 3);
    }
    for (int i = 0; i < 3; i++) {
        if (fiber_is_done(&fibers[counter])) {
            counter = ((counter + 1) % 3);
            continue;
        }
        fiber_continue(&fibers[counter]);
    }
}

void fiber_create(AsyncFiber* fiber, AsyncContext const* master, void* user, void(*callback)(AsyncFiberContext context, void* user))
{
    if (async_context_save(&fiber->context)) {
        return;
    }
    AsyncFiberContext context = {
        .master = master,
        .fiber = fiber,
    };
    callback(context, user);
    fiber->done = true;
    async_context_load(master);
}

void fiber_yield(AsyncFiberContext context)
{
    async_context_swap(&context.fiber->context, context.master);
}

int fiber_is_done(AsyncFiber const* fiber)
{
    return fiber->done;
}

void fiber_continue(AsyncFiber const* fiber)
{
    async_context_load(&fiber->context);
}

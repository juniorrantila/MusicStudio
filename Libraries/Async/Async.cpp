#include "./Async.h"
#include "Context.h"

#if 0
void async_trampoline(Async async);

AsyncFiber async_create(void* stack, usize stack_size, void* user, void(*callback)(Async, void* user))
{
}

void async_continue(AsyncFiber* fiber, AsyncContext const* master)
{
    if (fiber->state == 0) {
        fiber->state = 1;
        async_trampoline(fiber->async);
    }
    if (fiber->state == 1) {
        async_context_load(&fiber->context);
    }
    if (fiber->state == 2) {
        async_context_load(master);
    }
}

void async_yield(Async async)
{
    async_context_swap(&async.fiber->context, async.master);
}
#endif

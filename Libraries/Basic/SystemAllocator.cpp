#include "./SystemAllocator.h"

#include <stdlib.h>

static void* dispatch(Allocator*, AllocatorEvent event);

C_API Allocator* system_allocator(void)
{
    static thread_local Allocator a = allocator_init(dispatch);
    return &a;
}

static void* dispatch(Allocator*, AllocatorEvent event)
{
    switch (event.tag) {
    case AllocatorEventTag_Alloc: {
        void* ptr = 0;
        if (event.align < 16) event.align = 16;
        if (posix_memalign(&ptr, event.align, event.byte_count) != 0)
            return nullptr;
        return ptr;
    }
    case AllocatorEventTag_Free:
        free(event.ptr);
        return nullptr;
    case AllocatorEventTag_Owns:
        return 0;
    }
    return nullptr;
}

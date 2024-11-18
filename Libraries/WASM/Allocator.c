#include "./Allocator.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

void* allocator_alloc_impl(WASMAllocator* self, usize size, usize align)
{
    return self->alloc(self, size, align);
}

void allocator_free_impl(WASMAllocator* self, void* addr, usize size)
{
    self->free(self, addr, size);
}

static void* system_alloc(void*, usize size, usize align)
{
    if (align < sizeof(void*)) {
        align = sizeof(void*);
    }
    void* ptr = nullptr;
    int res = posix_memalign(&ptr, align, size);
    if (res != 0) {
        errno = res;
        return 0;
    }
    return ptr;
}

static void system_free(void*, void* ptr, usize)
{
    free(ptr);
}

WASMAllocator system_allocator = {
    .alloc = system_alloc,
    .free = system_free,
};

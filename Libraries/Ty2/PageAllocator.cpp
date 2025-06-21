#include "./PageAllocator.h"

#include "./Allocator.h"
#include "./Verify.h"

#include <sys/mman.h>
#include <unistd.h>

static void* dispatch(Allocator*, AllocatorEvent event);

C_API Allocator* page_allocator(void)
{
    static thread_local Allocator a = make_allocator(dispatch);
    return &a;
}

C_API usize page_size(void)
{
    static u32 size = 0;
    if (size == 0) {
        size = sysconf(_SC_PAGESIZE);
    }
    return size;
}

C_API void* page_alloc(usize size)
{
    void* ptr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED)
        return 0;
    if (mlock(ptr, size) < 0) {
        munmap(ptr, size);
        return 0;
    }
    return ptr;
}

C_API void page_free(void* ptr, usize size)
{
    munmap(ptr, size);
}


static void* dispatch_alloc(usize size, usize align)
{
    VERIFY(align <= page_size());
    return page_alloc(size);
}

static void dispatch_free(void* ptr, usize size, usize align)
{
    VERIFY(align <= page_size());
    page_free(ptr, size);
}


static bool dispatch_owns(void* ptr)
{
    (void)ptr;
    return true;
}

static void* dispatch(Allocator*, AllocatorEvent event)
{
    switch (event.tag) {
    case AllocatorEventTag_Alloc:
        return dispatch_alloc(event.byte_count, event.align);
    case AllocatorEventTag_Free:
        dispatch_free(event.ptr, event.byte_count, event.align);
        return nullptr;
    case AllocatorEventTag_Owns:
        return dispatch_owns(event.ptr) ? (void*)1 : 0;
    }
    return nullptr;
}

#include "./PageAllocator.h"

#include "./Allocator.h"
#include "./Verify.h"

#include <sys/mman.h>
#include <unistd.h>

static void* ialloc(Allocator* allocator, usize size, usize align);
static void ifree(Allocator* allocator, void* ptr, usize size);

Allocator* page_allocator(void)
{
    static Allocator allocator = (Allocator){
        .ialloc = ialloc,
        .ifree = ifree,
    };
    return &allocator;
}

static void* ialloc(Allocator*, usize size, usize align)
{
    static u32 page_size = 0;
    if (page_size == 0) {
        page_size = sysconf(_SC_PAGESIZE);
    }
    VERIFY(align <= page_size);
    void* ptr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
        return 0;
    }
    return ptr;
}

static void ifree(Allocator*, void* ptr, usize size)
{
    munmap(ptr, size);
}

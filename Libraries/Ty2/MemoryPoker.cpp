#include "./MemoryPoker.h"

#include "./PageAllocator.h"
#include "./Verify.h"

#include <pthread.h>

static consteval struct timespec operator ""_ms(unsigned long long value)
{
    long long v = (long long)value;
    struct timespec time = {
        .tv_sec = 0,
        .tv_nsec = v * 1000000,
    };
    while (v >= 1000) {
        time.tv_sec += 1;
        v /= 1000;
    }
    time.tv_nsec = v * 1000000LL;
    return (struct timespec){
        .tv_sec = 0,
        .tv_nsec = v * 1000000,
    };
}

static void* poker_thread(void*);

C_API [[nodiscard]] bool memory_poker_init(MemoryPoker* poker)
{
    memzero(poker, sizeof(*poker));

    pthread_t thread;
    int res = pthread_create(&thread, nullptr, poker_thread, poker);
    if (res != 0) return false;
    pthread_detach(thread);

    return true;
}

void MemoryPoker::push(void const* memory, u64 size) { return memory_poker_push(this, memory, size); }
C_API void memory_poker_push(MemoryPoker* poker, void const* memory, u64 size)
{
    u64 page_size = ::page_size();
    u64 start = ((u64)(uptr)memory) & ~page_size;
    u64 page_count = size / page_size;
    u32 truncated_page_count = page_count;
    VERIFY(page_count == truncated_page_count);

    u64 new_count = poker->count + 1;
    VERIFY(new_count < memory_poker_ranges_max);
    u64 id = new_count - 1;
    poker->pages[id] = (u8 const*)(uptr)start;
    poker->page_counts[id] = page_count;
    poker->count = new_count;
}

[[clang::no_sanitize("address")]]
static void* poker_thread(void* user)
{
    pthread_setname_np("memory-poker");

    MemoryPoker const* poker = (MemoryPoker const*)user; 
    u64 page_size = ::page_size();
    for (;;) {
        u64 count = poker->count;
        for (u64 i = 0; i < count; i++) {
            u8 volatile const* start = poker->pages[i];
            u64 page_count = poker->page_counts[i];

            start[0];
            for (u64 page = 0; page < page_count; page++)
                start[(page + 1) * page_size];
        }

        auto interval = 500_ms;
        nanosleep(&interval, nullptr);
    }

    UNREACHABLE();
    return nullptr;
}


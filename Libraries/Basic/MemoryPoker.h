#pragma once
#include "./Base.h"
#include "./Bits.h"

// NOTE: Only thread safe for single consumer single producer
constexpr u64 memory_poker_ranges_max = 128LLU;
typedef struct MemoryPoker {
    u64 version;

    _Atomic u64 count;
    u8 volatile const* pages[memory_poker_ranges_max];
    u32 page_counts[memory_poker_ranges_max];

#if __cplusplus
    void push(void const*, u64);

    [[nodiscard]] bool start();
#endif
} MemoryPoker;
static_assert(sizeof(MemoryPoker) < 2 * KiB);

C_API void memory_poker_init(MemoryPoker*);
C_API void memory_poker_push(MemoryPoker*, void const*, u64);
C_API [[nodiscard]] bool memory_poker_start(MemoryPoker*);

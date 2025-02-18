#pragma once
#include "./Allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Allocator allocator;
    u8* base;
    u8* head;
    u8* end;
} FixedArena;

FixedArena fixed_arena_from_slice(void* memory, usize size);
usize fixed_arena_bytes_used(FixedArena const*);

void fixed_arena_drain(FixedArena*);
void* fixed_arena_mark(FixedArena const*);
void fixed_arena_sweep(FixedArena*, void* mark);
bool fixed_arena_owns(FixedArena const*, void* ptr);

#ifdef __cplusplus
}
#endif

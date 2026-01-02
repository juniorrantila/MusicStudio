#pragma once
#include "./Types.h"

#include "./Error.h"

typedef struct RingBuffer {
    u64 version;

    u8* items;
    _Atomic u32 read_offset;
    _Atomic u32 write_offset;
    u32 capacity;
} RingBuffer;

C_API KError ring_buffer_init(RingBuffer*, u32 min_capacity);
C_API KError ring_buffer_lazy_init(RingBuffer*, u32 min_capacity);
C_API KError ring_buffer_ensure_initialized(RingBuffer*);
C_API void ring_buffer_deinit(RingBuffer*);

C_API u8* ring_buffer_write_ptr(RingBuffer const*);
C_API u8 const* ring_buffer_read_ptr(RingBuffer const*);

C_API u32 ring_buffer_size(RingBuffer const*);
C_API u32 ring_buffer_size_left(RingBuffer const*);
C_API void ring_buffer_produce(RingBuffer*, u32 bytes);
C_API void ring_buffer_consume(RingBuffer*, u32 bytes);

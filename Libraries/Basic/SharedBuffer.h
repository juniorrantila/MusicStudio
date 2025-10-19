#pragma once
#include "./Forward.h"
#include "./Types.h"
#include "./Error.h"

#ifndef NDEBUG
#include <pthread.h>
#endif

#ifdef __cplusplus
#include "./TypeId.h"
#include "./Bits.h"
#endif

typedef struct SharedBuffer {
    u64 version; // sizeof(*this)

    u8* buffer;
    u64 capacity;
    u64 head;
    u64 tail;

#ifndef NDEBUG
    pthread_t reader;
    pthread_t writer;
    u32 reader_is_bound;
    u32 writer_is_bound;
#endif
} SharedBuffer;

typedef struct Sink Sink;
typedef struct Source Source;

C_API KError  shared_buffer_init(SharedBuffer*, u64 minimum_capacity IF_CPP(= 16 * KiB));
C_API void    shared_buffer_deinit(SharedBuffer*);
C_API Sink*   shared_buffer_sink(SharedBuffer*);
C_API Source* shared_buffer_source(SharedBuffer*);

#ifndef NDEBUG
C_API void sink_unbind(Sink*);
C_API void source_unbind(Source*);
C_API c_string source_file(Source*);
C_API u32 source_line(Source*);
#else
C_INLINE void sink_unbind(Sink* sink) { (void)sink; }
C_INLINE void source_unbind(Source* source) { (void)source; }
C_INLINE c_string source_file(Source*) { return nullptr; }
C_INLINE u32 source_line(Source*) { return 0; }
#endif

#ifdef NDEBUG
C_API KError sink_write(Sink*, u16 tag, void const*, u64 size, u64 align);
#else
C_API KError sink_write(Sink*, u16 tag, void const*, u64 size, u64 align, c_string file, u32 line);
#ifndef __cplusplus
#define sink_write(sink, tag, data, size, align) sink_write(sink, tag, data, size, align, __FILE__, __LINE__)
#endif
#endif

C_API void source_wait(Source*);
C_API void source_next(Source*);

C_API bool source_peek(Source*, u16* tag IF_CPP(= nullptr), u64* size IF_CPP(= nullptr), u64* align IF_CPP(= nullptr));
C_API KError source_read(Source*, u16 tag, void*, u64 size, u64 align);

#ifdef __cplusplus
#ifdef NDEBUG
template <typename T>
static KError sink_write(Sink* sink, T const& value) { return sink_write(sink, Ty2::type_id<T>(), &value, sizeof(T), alignof(T)); }
#else
template <typename T>
static KError sink_write(Sink* sink, T const& value, c_string file = __builtin_FILE(), u32 line = __builtin_LINE()) { return sink_write(sink, Ty2::type_id<T>(), &value, sizeof(T), alignof(T), file, line); }
#endif

template <typename T>
static KError source_read(Source* source, T* out) { return source_read(source, Ty2::type_id<T>(), out, sizeof(T), alignof(T)); }
#endif

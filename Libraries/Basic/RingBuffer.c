#include "./RingBuffer.h"

#include "./Platform.h"
#include "./PageAllocator.h"
#include "./Bits.h"

#include "./Try.h"

#if PLATFORM_POSIX
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif

#if PLATFORM_POSIX

C_API KError ring_buffer_init(RingBuffer* rb, u32 min_capacity)
{
    TRY(ring_buffer_lazy_init(rb, min_capacity));
    TRY(ring_buffer_ensure_initialized(rb));
    return kerror_none;
}

C_API KError ring_buffer_lazy_init(RingBuffer* rb, u32 min_capacity)
{
    if (!C_ASSERT(!ty_is_initialized(rb))) return kerror_unix(EINVAL);
    if (!C_ASSERT(min_capacity > 0)) return kerror_unix(EINVAL);
    u32 actual_capacity = (min_capacity + page_size() - 1) & ~(page_size() - 1);
    *rb = (RingBuffer){
        .version = sizeof(RingBuffer),
        .items = nullptr,
        .read_offset = 0u,
        .write_offset = 0u,
        .capacity = actual_capacity,
    };
    return kerror_none;
}

C_API KError ring_buffer_ensure_initialized(RingBuffer* rb)
{
    if (!C_ASSERT(ty_is_initialized(rb))) return kerror_unix(EINVAL);
    if (!C_ASSERT(rb->capacity > 0)) return kerror_unix(EINVAL);
    if (rb->items) return kerror_none;

    char tmp_path[] = "/tmp/basic-ring-buffer-XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0) return kerror_unix(errno);

    if (unlink(tmp_path) < 0) {
        close(fd);
        return kerror_unix(errno);
    }

    if (ftruncate(fd, rb->capacity) < 0) {
        close(fd);
        return kerror_unix(errno);
    }

    u8* address = (u8*)mmap(NULL, 2 * (u64)rb->capacity, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (address == MAP_FAILED) {
        close(fd);
        return kerror_unix(errno);
    }

    u8* other_address = (u8*)mmap(address, rb->capacity, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address) {
        munmap(address, 2 * (u64)rb->capacity);
        close(fd);
        return kerror_unix(errno);
    }

    other_address = (u8*)mmap(address + rb->capacity, rb->capacity, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address + rb->capacity) {
        munmap(address, 2 * (u64)rb->capacity);
        close(fd);
        return kerror_unix(errno);
    }

    if (close(fd) < 0) {
        munmap(address, 2 * (u64)rb->capacity);
        return kerror_unix(errno);
    }

    rb->items = address;
    return kerror_none;
}

C_API void ring_buffer_deinit(RingBuffer* rb)
{
    rb->read_offset = rb->write_offset;
    if (rb->items) munmap(rb->items, 2 * (u64)rb->capacity);
    rb->items = nullptr;
    rb->version = 0;
}

C_API u8* ring_buffer_write_ptr(RingBuffer const* rb)
{
    if (!C_ASSERT(ty_is_initialized(rb))) return nullptr;
    if (!C_ASSERT(rb->items != nullptr && "you need to call ring_buffer_ensure_initialized if using lazy initialization")) {
        if (!ring_buffer_ensure_initialized((RingBuffer*)rb).ok) return nullptr;
    }
    return rb->items + (rb->write_offset % rb->capacity);
}

C_API u8 const* ring_buffer_read_ptr(RingBuffer const* rb)
{
    if (!C_ASSERT(ty_is_initialized(rb))) return nullptr;
    if (!C_ASSERT(rb->items != nullptr && "you need to call ring_buffer_ensure_initialized if using lazy initialization")) {
        if (!ring_buffer_ensure_initialized((RingBuffer*)rb).ok) return nullptr;
    }
    return rb->items + (rb->read_offset % rb->capacity);
}

C_API u32 ring_buffer_size(RingBuffer const* rb)
{
    i64 read_offset = rb->read_offset;
    i64 write_offset = rb->write_offset;
    i64 count = write_offset - read_offset;
    if (!C_ASSERT(count >= 0)) return 0;
    if (!C_ASSERT(count <= rb->capacity)) return rb->capacity;
    return (u32)count;
}

C_API u32 ring_buffer_size_left(RingBuffer const* rb)
{
    return rb->capacity - ring_buffer_size(rb);
}

C_API void ring_buffer_produce(RingBuffer* rb, u32 bytes)
{
    if (!C_ASSERT(bytes < rb->capacity)) return;
    if (!C_ASSERT(rb->items != nullptr)) return;
    rb->write_offset += bytes;
}

C_API void ring_buffer_consume(RingBuffer* rb, u32 bytes)
{
    if (!C_ASSERT(bytes < rb->capacity)) return;
    if (!C_ASSERT(rb->items != nullptr)) return;
    rb->read_offset += bytes;
}

#else
#error "unsupported OS"
#endif

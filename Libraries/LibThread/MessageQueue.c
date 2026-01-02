#include "./MessageQueue.h"

#include <Basic/Verify.h>
#include <Basic/Context.h>
#include <Basic/Allocator.h>
#include <Basic/Bits.h>
#include <Basic/BitSet.h>
#include <Basic/Try.h>

#include <errno.h>

typedef struct [[gnu::packed]] Header {
    u64 size;
    THMessageKind kind;
    u16 align;
    u8 data[];
} Header;

C_API KError th_message_queue_init(THMessageQueue* q, u64 min_capacity)
{
    if (!C_ASSERT(q != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(!ty_is_initialized(q))) return kerror_unix(EINVAL);
    if (!C_ASSERT(min_capacity != 0)) return kerror_unix(EINVAL);

    MEMZERO(q);
    KError error = ring_buffer_init(&q->buffer, min_capacity);
    if (!error.ok) return error;
    th_sem_init(&q->messages, 0);

    ty_set_initialized(q);
    return kerror_none;
}

C_API KError th_message_queue_lazy_init(THMessageQueue* q, u64 min_capacity)
{
    if (!C_ASSERT(q != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(!ty_is_initialized(q))) return kerror_unix(EINVAL);
    if (!C_ASSERT(min_capacity != 0)) return kerror_unix(EINVAL);

    MEMZERO(q);
    KError error = ring_buffer_lazy_init(&q->buffer, min_capacity);
    if (!error.ok) return error;
    th_sem_init(&q->messages, 0);

    ty_set_initialized(q);
    return kerror_none;
}

C_API void th_message_queue_deinit(THMessageQueue* q)
{
    if (ty_is_initialized(q)) {
        ring_buffer_deinit(&q->buffer);
        th_sem_deinit(&q->messages);
        q->version = 0;
    }
}

C_API KError th_message_send(THMessageQueue* q, THMessageKind kind, void const* data, u64 size, u16 align)
{
    if (!C_ASSERT(q != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(ty_is_initialized(q))) return kerror_unix(EINVAL);
    if (!C_ASSERT(data != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(size != 0)) return kerror_unix(EINVAL);
    if (!C_ASSERT(align != 0)) return kerror_unix(EINVAL);
    if (!C_ASSERT(u64_popcount(align) == 1)) return kerror_unix(EINVAL);

    if (!kthread_id_is_valid(q->writer)) q->writer = current_thread_id();
    if (!C_ASSERT(kthread_id_equal(q->writer, current_thread_id()))) return kerror_unix(EINVAL);

    TRY(ring_buffer_ensure_initialized(&q->buffer));

    u64 bytes = size + sizeof(THMessage);
    if (ring_buffer_size_left(&q->buffer) < bytes)
        return kerror_unix(EAGAIN);

    Header* message = (Header*)ring_buffer_write_ptr(&q->buffer);
    message->kind = kind;
    message->size = size;
    message->align = align;
    memcpy(message->data, data, size);
    ring_buffer_produce(&q->buffer, bytes);
    th_sem_signal(&q->messages);

    return kerror_none;
}

C_API [[nodiscard]] bool th_message_try_read(THMessageQueue* q, THMessage* out)
{
    if (!C_ASSERT(q != nullptr)) return false;
    if (!C_ASSERT(ty_is_initialized(q))) return false;
    if (!C_ASSERT(out != nullptr)) return false;

    if (!kthread_id_is_valid(q->reader)) q->reader = current_thread_id();
    if (!C_ASSERT(kthread_id_equal(q->reader, current_thread_id()))) return false;

    KError error = ring_buffer_ensure_initialized(&q->buffer);
    if (!error.ok) {
        errorf("could not lazy initialize ring buffer: %s", kerror_tstring(error));
        return false;
    }

    MEMZERO(out);

    u64 buffer_size = ring_buffer_size(&q->buffer);
    if (buffer_size == 0) return false;
    if (!C_ASSERT(buffer_size > sizeof(THMessage))) return false;

    Header* ptr = (Header*)ring_buffer_read_ptr(&q->buffer);
    u64 size = ptr->size;
    u16 align = ptr->align;
    THMessageKind kind = ptr->kind;
    void* data = tclone(ptr->data, size, align);
    if (!C_ASSERT(data != nullptr)) return false;
    ring_buffer_consume(&q->buffer, sizeof(THMessage) + size);
    VERIFY(th_sem_wait_for(&q->messages, 0).ok);

    *out = (THMessage){
        .size = size,
        .data = data,
        .kind = kind,
        .align = align,
    };
    return true;
}

C_API KError th_message_wait(THMessageQueue* q, THMessage* out)
{
    if (!C_ASSERT(q != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(ty_is_initialized(q))) return kerror_unix(EINVAL);
    if (!C_ASSERT(out != nullptr)) return kerror_unix(EINVAL);

    if (!kthread_id_is_valid(q->reader)) q->reader = current_thread_id();
    if (!C_ASSERT(kthread_id_equal(q->reader, current_thread_id()))) return kerror_unix(EINVAL);

    MEMZERO(out);

    KError error = th_sem_wait(&q->messages);
    if (!error.ok) return error;

    u64 buffer_size = ring_buffer_size(&q->buffer);
    if (!C_ASSERT(buffer_size > sizeof(THMessage))) {
        th_sem_signal(&q->messages);
        return kerror_unix(EINVAL);
    }

    Header* ptr = (Header*)ring_buffer_read_ptr(&q->buffer);
    u64 size = ptr->size;
    u16 align = ptr->align;
    THMessageKind kind = ptr->kind;
    void* data = tclone(ptr->data, size, align);
    if (!C_ASSERT(data != nullptr)) {
        th_sem_signal(&q->messages);
        return kerror_unix(ENOMEM);
    }
    ring_buffer_consume(&q->buffer, sizeof(THMessage) + size);

    *out = (THMessage){
        .size = size,
        .data = data,
        .kind = kind,
        .align = align,
    };
    return kerror_none;
}

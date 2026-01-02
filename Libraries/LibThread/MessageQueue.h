#pragma once
#include "./Forward.h"

#include "./Semaphore.h"

#include <Basic/Types.h>
#include <Basic/Error.h>
#include <Basic/RingBuffer.h>

typedef union { u64 tag; char name[8]; } THMessageKind;
#define TH_MESSAGE_KIND(name) ((THMessageKind){CSTRING_U64(name)})

typedef struct THMessageQueue THMessageQueue;
typedef struct THMessage THMessage;

typedef struct THMessageQueue {
    u64 version;

    RingBuffer buffer;
    THSemaphore messages;

    KThreadID reader;
    KThreadID writer;
} THMessageQueue;

typedef struct THMessage {
    u64 size;
    void* data;
    THMessageKind kind;
    u16 align;
} THMessage;

C_API KError th_message_queue_init(THMessageQueue*, u64 min_capacity);
C_API KError th_message_queue_lazy_init(THMessageQueue*, u64 min_capacity);
C_API void th_message_queue_deinit(THMessageQueue*);

C_API KError th_message_send(THMessageQueue*, THMessageKind, void const*, u64 size, u16 align);
#define TH_MESSAGE_SEND(q, kind, data) th_message_send(q, kind, data, sizeof(*data), alignof(__typeof(*data)))

#ifdef __cplusplus
template <typename T>
    requires (T::kind.tag != 0)
static inline KError th_message_send(THMessageQueue* q, T const& data) { return th_message_send(q, T::kind, &data, sizeof(T), alignof(T)); }
#endif
C_API [[nodiscard]] bool th_message_try_read(THMessageQueue*, THMessage*);
C_API KError th_message_wait(THMessageQueue*, THMessage*);

#pragma once
#include "./Base.h"

#include "./Allocator.h"
#include "./Mailbox.h"

typedef struct [[nodiscard]] { bool ok; } DispatchStatus;
C_INLINE DispatchStatus dispatch_ok() { return (DispatchStatus){true}; }
C_INLINE DispatchStatus dispatch_fail() { return (DispatchStatus){false}; }

typedef struct DispatchWorker {
    pthread_t pthread ;
    Mailbox thread;
    Mailbox main;
} DispatchWorker;

typedef struct DispatchQueue {
    DispatchWorker* workers;
    u64 count;

    u64 robin;
    u64 running;

#ifdef __cplusplus
    void dispatch(void* user1, void* user2, u64 size, u64 batch_size, void(*)(u64 worker, void* user1, void* user2, u64 begin, u64 end));

    void sync();
#endif
} DispatchQueue;

C_API DispatchStatus dispatch_queue_init(Allocator*, u64 min_mailbox_capacity, DispatchQueue*);
C_API void dispatch_queue(DispatchQueue*, void* user1, void* user2, u64 size, u64 batch_size, void(*)(u64 worker, void* user1, void* user2, u64 begin, u64 end));
C_API void dispatch_queue_sync(DispatchQueue*);

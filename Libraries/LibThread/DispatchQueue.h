#pragma once
#include "./Thread.h"
#include "./Semaphore.h"

typedef struct THDispatchWork {
    u64 start;
    u64 end;
    void* user1;
    void* user2;
    void (*callback)(u64 worker, void* user1, void* user2, u64 begin, u64 end);
} THDispatchWork;

typedef struct THDispatchWorker {
    u64 worker_id;
    THThread thread;

    THSemaphore there_is_work;
    THSemaphore completed_work;
    _Atomic u64 work_count;
    _Atomic u64 done_count;
    THDispatchWork queue[256];
} THDispatchWorker;

typedef struct THDispatchQueue {
    u64 version;
    u64 robin;
    u64 worker_count;
    THDispatchWorker workers[256];
} THDispatchQueue;
static_assert(sizeof(THDispatchQueue) == 2654232);

C_API KError th_dispatch_queue_init(THDispatchQueue* queue, c_string name);
C_API void th_dispatch_queue(THDispatchQueue*, void* user1, void* user2, u64 size, u64 batch_size, void(*)(u64 worker, void* user1, void* user2, u64 begin, u64 end));
C_API void th_dispatch_sync(THDispatchQueue*);

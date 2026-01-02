#include "./DispatchQueue.h"

#include <Basic/Verify.h>
#include <Basic/Bits.h>
#include <Basic/Allocator.h>
#include <Basic/Try.h>

#include <unistd.h>
#include <errno.h>

static void dispatch_thread(void*);

C_API KError th_dispatch_queue_init(THDispatchQueue* q, c_string name)
{
    if (!C_ASSERT(q != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(!ty_is_initialized(q))) return kerror_unix(EINVAL);

    MEMZERO(q);

    i64 worker_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (worker_count < 0) return kerror_unix(errno);
    if (!C_ASSERT(worker_count < ARRAY_SSIZE(q->workers))) {
        worker_count = ARRAY_SIZE(q->workers) - 1;
    }

    q->worker_count = (u64)worker_count;

    for (i64 i = 0; i < worker_count; i++) {
        THDispatchWorker* worker = &q->workers[i];
        worker->worker_id = i + 1;
        TRY(th_thread_init(&worker->thread, name, (Context){}, worker, dispatch_thread));
        th_sem_init(&worker->there_is_work, 0);
        th_sem_init(&worker->completed_work, 0);
    }

    q->version = sizeof(*q);
    return kerror_none;
}

C_API void th_dispatch_queue(THDispatchQueue* q, void* user1, void* user2, u64 size, u64 batch_size, void(*callback)(u64 worker, void* user1, void* user2, u64 begin, u64 end))
{
    if (!C_ASSERT(q)) return;
    if (!C_ASSERT(ty_is_initialized(q))) return;
    if (!C_ASSERT(batch_size > 0)) return;

    for (u64 i = 0; i < size; i += batch_size) {
        u64 worker_id = q->robin++ % q->worker_count;
        THDispatchWorker* worker = &q->workers[worker_id];
        THDispatchWork work = {
            .start = i,
            .end = i + batch_size < size ? i + batch_size : size,
            .user1 = user1,
            .user2 = user2,
            .callback = callback,
        };
        u64 size_left = ARRAY_SIZE(worker->queue) - (worker->work_count - worker->done_count);
        if (size_left >= 1) {
            th_thread_start(&worker->thread);
            worker->queue[worker->work_count++ % ARRAY_SIZE(worker->queue)] = work;
            th_sem_signal(&worker->there_is_work);
        } else {
            callback(0, user1, user2, work.start, work.end);
        }
    }
}

C_API void th_dispatch_sync(THDispatchQueue* q)
{
    if (!C_ASSERT(q)) return;
    if (!C_ASSERT(ty_is_initialized(q))) return;

    for (u64 i = 0; i < q->worker_count; i++) {
        THDispatchWorker* worker = &q->workers[i];
        while (worker->done_count != worker->work_count) {
            KError err = th_sem_wait(&worker->completed_work);
            if (!err.ok) continue;
        }
    }
}

static void dispatch_thread(void* ptr)
{
    THDispatchWorker* ctx = (THDispatchWorker*)ptr;

    for (;;) {
        reset_temporary_arena();
        KError err = th_sem_wait(&ctx->there_is_work);
        if (!err.ok) continue;
        VERIFY(ctx->done_count < ctx->work_count);
        u64 index = ctx->done_count % ARRAY_SIZE(ctx->queue);
        THDispatchWork work = ctx->queue[index];
        if (!C_ASSERT(work.callback != nullptr));
        else work.callback(ctx->worker_id, work.user1, work.user2, work.start, work.end);
        work.callback = nullptr;
        ctx->done_count++;
        th_sem_signal(&ctx->completed_work);
    }

    UNREACHABLE();
}

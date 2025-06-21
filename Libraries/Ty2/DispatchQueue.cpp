#include "./DispatchQueue.h"

#include "./Allocator.h"
#include "./Mailbox.h"
#include "./Verify.h"
#include "./Defer.h"

#include <pthread.h>
#include <sys/signal.h>
#include <unistd.h>

static void* dispatch_thread(void*);

struct Work {
    u64 worker;
    u64 start;
    u64 end;
    void* user1;
    void* user2;
    void (*callback)(u64 worker, void* user1, void* user2, u64 begin, u64 end);
};

struct Done {
    u64 failed_posts;
};

C_API DispatchStatus dispatch_queue_init(Allocator* a, u64 mailbox_capacity, DispatchQueue* out)
{
    i64 worker_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (worker_count < 0) return dispatch_fail();

    i64 active_threads = 0;
    auto* workers = (DispatchWorker*)memalloc(a, worker_count * sizeof(DispatchWorker), alignof(DispatchWorker));
    if (!workers) return dispatch_fail();
    Defer destroy_workers = [&]{
        for (i64 thread = 0; thread < active_threads; thread++) {
            pthread_kill(workers[thread].pthread, SIGKILL);
        }
        memfree(a, workers, worker_count * sizeof(DispatchWorker), alignof(DispatchWorker));
    };

    for (; active_threads < worker_count; active_threads++) {
        auto* worker = &workers[active_threads];
        *worker = (DispatchWorker){
            .pthread = {},
            .thread = {},
            .main = {},
        };
        if (!mailbox_init(mailbox_capacity, &worker->thread).ok)
            return dispatch_fail();
        if (!mailbox_init(mailbox_capacity, &worker->main).ok)
            return dispatch_fail();
        if (pthread_create(&worker->pthread, nullptr, dispatch_thread, worker) != 0)
            return dispatch_fail();
    }

    destroy_workers.disarm();
    *out = (DispatchQueue){
        .workers = workers,
        .count = (u64)worker_count,
        .robin = 0,
        .running = 0,
    };
    return dispatch_ok();
}

void DispatchQueue::dispatch(void* user1, void* user2, u64 size, u64 batch_size, void(*callback)(u64 worker, void* user1, void* user2, u64 begin, u64 end)) { return dispatch_queue(this, user1, user2, size, batch_size, callback); }
C_API void dispatch_queue(DispatchQueue* q, void* user1, void* user2, u64 size, u64 batch_size, void(*callback)(u64 worker, void* user1, void* user2, u64 begin, u64 end))
{
    VERIFY(batch_size > 0);

    for (u64 i = 0; i < size; i += batch_size) {
        u64 worker = q->robin++ % q->count;
        auto status = q->workers[worker].thread.writer()->post(Work{
            .worker = worker + 1,
            .start = i,
            .end = i + batch_size < size ? i + batch_size : size,
            .user1 = user1,
            .user2 = user2,
            .callback = callback,
        });
        if (status.ok) ++q->running;
        else callback(0, user1, user2, i, i + batch_size < size ? i + batch_size : size);
    }
}

void DispatchQueue::sync() { return dispatch_queue_sync(this); }
C_API void dispatch_queue_sync(DispatchQueue* q)
{
    auto ms = [](u64 value) -> struct timespec {
        struct timespec time = {
            .tv_sec = 0,
            .tv_nsec = (long long)(value * 1000000),
        };
        while (value >= 1000) {
            time.tv_sec += 1;
            value /= 1000;
        }
        time.tv_nsec = (long long)(value * 1000000LL);
        return (struct timespec){
            .tv_sec = 0,
            .tv_nsec = (long long)(value * 1000000),
        };
    };
    while (q->running > 0) {
        mailbox_wait_any(ms(16));
        __builtin_printf("%zu\n", q->running);
        for (u64 i = 0; i < q->count; i++) {
            Message message;
            if (q->workers[i].main.reader()->read(&message).found) {
                q->running -= 1;

                Done done;
                VERIFY(message.unwrap(&done).ok);
                q->running -= done.failed_posts;
            }
        }
    }
}

static void* dispatch_thread(void* ptr)
{
    auto ms = [](u64 value) -> struct timespec {
        struct timespec time = {
            .tv_sec = 0,
            .tv_nsec = (long long)(value * 1000000),
        };
        while (value >= 1000) {
            time.tv_sec += 1;
            value /= 1000;
        }
        time.tv_nsec = (long long)(value * 1000000LL);
        return (struct timespec){
            .tv_sec = 0,
            .tv_nsec = (long long)(value * 1000000),
        };
    };

    auto* ctx = (DispatchWorker*)ptr;

    u64 failed = 0;
    for (;;) {
        auto* reader = ctx->thread.reader();
        reader->wait(ms(16));
        Message message;
        while (reader->read(&message).found) {
            Work work;
            VERIFY(message.unwrap(&work).ok);
            work.callback(work.worker, work.user1, work.user2, work.start, work.end);
            if (ctx->main.writer()->post(Done{.failed_posts = failed}).ok) failed = 0;
            else failed += 1;
        }
    }

    return nullptr;
}

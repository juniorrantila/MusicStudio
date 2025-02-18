#include "./Schedule.h"

#include <Ty/SmallCapture.h>

#include <mach/mach_init.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <pthread.h>
#include <sys/sysctl.h>

struct Schedule {
    struct VLA {
        Schedule* schedule;
        pthread_t thread;
    };

    Allocator* arena;

    void* runner_context;
    void (*runner)(void*, u32 thread_id);

    pthread_mutex_t threads_done;
    _Atomic u32 thread_done;
    u32 thread_count;
    _Atomic bool should_die;
    VLA threads[];
};

static void resume(Schedule* schedule);

static void* thread_proc(void* context);

e_schedule schedule_create(Schedule** out_schedule, Allocator* arena)
{
    e_schedule error = e_sched_none;
    u32 thread_count = 0;
    int physical_cpu = 0;
    size_t sizeof_int = sizeof(int);
    usize initialized_threads = 0;
    Schedule* schedule = nullptr;

    if (sysctlbyname("hw.physicalcpu", &physical_cpu, &sizeof_int, nullptr, 0) < 0) {
        error = e_sched_could_not_get_hardware_concurrency;
        goto fi_0;
    }
    thread_count = (u32)physical_cpu;

    schedule = arena->alloc_vla<Schedule>(thread_count).or_default(nullptr);
    if (!schedule) {
        error = e_sched_out_of_memory;
        goto fi_1;
    }
    *schedule = Schedule {
        .arena = arena,
        .runner_context = nullptr,
        .runner = nullptr,
        .threads_done = PTHREAD_MUTEX_INITIALIZER,
        .thread_done = 0u,
        .thread_count = thread_count,
        .should_die = false,
    };

    for (initialized_threads = 0; initialized_threads < thread_count; initialized_threads++) {
        Schedule::VLA* context = &schedule->threads[initialized_threads];
        context->schedule = schedule;

        usize i = initialized_threads;
        if (pthread_create_suspended_np(&schedule->threads[i].thread, nullptr, thread_proc, context) != 0)
            goto fi_2;
    }

    pthread_mutex_lock(&schedule->threads_done);
    *out_schedule = schedule;
    return e_sched_none;
fi_2:
    schedule->should_die = true;
    for (usize i = 0; i < initialized_threads; i++) {
        mach_port_t port = pthread_mach_thread_np(schedule->threads[i].thread);
        thread_resume(port);
        pthread_join(schedule->threads[i].thread, nullptr);
    }
fi_1:
    arena->free_vla(schedule, thread_count);
fi_0:
    return error;
}


void schedule_destroy(Schedule* schedule)
{
    schedule->should_die = true;
    resume(schedule);
    for (usize i = 0; i < schedule->thread_count; i++) {
        pthread_join(schedule->threads[i].thread, nullptr);
    }
    schedule->arena->free_vla(schedule, schedule->thread_count);
}


void schedule_chunked_for(
    Schedule* schedule,
    usize items,
    void* user,
    void(*callback)(void* user, usize begin, usize end, u32 thread_id)
) {
    const usize work_size = items / schedule->thread_count;
    const usize last_thread_size = work_size + (items - (work_size * schedule->thread_count));

    struct Context {
        void* user;
        void(*callback)(void* user, usize begin, usize end, u32 thread_id);
        usize work_size;
        usize last_thread_size;
        u32 thread_count;
    } context = {
        .user = user,
        .callback = callback,
        .work_size = work_size,
        .last_thread_size = last_thread_size,
        .thread_count = schedule->thread_count,
    };
    schedule->runner_context = &context;
    schedule->runner = [](void* user, u32 thread_id) {
        auto* ctx = (Context*)user;
        usize size = ctx->work_size;
        if (thread_id == ctx->thread_count - 1) {
            size = ctx->last_thread_size;
        }
        const usize start = ctx->work_size * thread_id;
        const usize end = start + size;
        ctx->callback(ctx->user, start, end, thread_id);
    };

    resume(schedule);
    pthread_mutex_lock(&schedule->threads_done);
}

void schedule_parallel_for(
    Schedule* schedule,
    usize items,
    void* user,
    void(* callback)(void* user, usize index, u32 thread_id)
) {
    const usize work_size = items / schedule->thread_count;
    const usize last_thread_size = work_size + (items - (work_size * schedule->thread_count));
    struct Context {
        void* user;
        void(*callback)(void* user, usize index, u32 thread_id);
        usize work_size;
        usize last_thread_size;
        u32 thread_count;
    } context = {
        .user = user,
        .callback = callback,
        .work_size = work_size,
        .last_thread_size = last_thread_size,
        .thread_count = schedule->thread_count,
    };
    schedule->runner_context = &context;
    schedule->runner = [](void* context, u32 thread_id) {
        auto* ctx = (Context*)context;
        usize size = ctx->work_size;
        if (thread_id == ctx->thread_count - 1) {
            size = ctx->last_thread_size;
        }
        const usize start = ctx->work_size * thread_id;
        const usize end = start + size;
        for (usize i = start; i < end; i++) {
            ctx->callback(ctx->user, i, thread_id);
        }
    };

    resume(schedule);
    pthread_mutex_lock(&schedule->threads_done);
}

static void* thread_proc(void* context)
{
    auto* th = (Schedule::VLA*)context;
    Schedule* schedule = th->schedule;
    usize id = ((uptr)th - (uptr)schedule->threads) / sizeof(Schedule::VLA);
    while (true) {
        thread_suspend(mach_thread_self());
        if (schedule->should_die) {
            break;
        }
        schedule->runner(schedule->runner_context, id);
        if (++schedule->thread_done == schedule->thread_count) {
            schedule->thread_done = 0;
            pthread_mutex_unlock(&schedule->threads_done);
        }
    }
    return nullptr;
}

static void resume(Schedule* schedule)
{
    for (usize i = 0; i < schedule->thread_count; i++) {
        mach_port_t port = pthread_mach_thread_np(schedule->threads[i].thread);
        thread_resume(port);
    }
}

c_string schedule_strerror(e_schedule error)
{
    switch (error) {
    case e_sched_none: return "no error";
    case e_sched_could_not_get_hardware_concurrency: return "could not get hardware concurrency";
    case e_sched_could_not_create_threads: return "could not create threads";
    case e_sched_out_of_memory: return "out of memory";
    }
}

#include "./Schedule.h"

#include <Ty/SmallCapture.h>

#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <pthread.h>
#include <sys/sysctl.h>

struct Schedule {
    struct VLA {
        Schedule* schedule;
        pthread_t thread;
    };

    Allocator* arena;
    SmallCapture<void(u32 thread_id)> runner;

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
    schedule->runner = [=](u32 thread_id) {
        usize size = work_size;
        if (thread_id == schedule->thread_count - 1) {
            size = last_thread_size;
        }
        const usize start = work_size * thread_id;
        const usize end = start + size;
        callback(user, start, end, thread_id);
    };

    schedule->thread_done = 0;
    resume(schedule);
    pthread_mutex_lock(&schedule->threads_done);
}

void schedule_parallel_for(Schedule* schedule, usize items, void* user, void(* callback)(void* user, usize index, u32 thread_id))
{
    const usize work_size = items / schedule->thread_count;
    const usize last_thread_size = work_size + (items - (work_size * schedule->thread_count));
    schedule->runner = [=](u32 thread_id) {
        usize size = work_size;
        if (thread_id == schedule->thread_count - 1) {
            size = last_thread_size;
        }
        const usize start = work_size * thread_id;
        const usize end = start + size;
        for (usize i = start; i < end; i++) {
            callback(user, i, thread_id);
        }
    };

    schedule->thread_done = 0;
    resume(schedule);
    pthread_mutex_lock(&schedule->threads_done);
}

static void* thread_proc(void* context)
{
    auto* th = (Schedule::VLA*)context;
    Schedule* schedule = th->schedule;
    usize id = ((uptr)th - (uptr)schedule->threads) / sizeof(Schedule::VLA);
    while (true) {
        if (schedule->should_die) {
            break;
        }
        schedule->runner(id);
        if (++schedule->thread_done == schedule->thread_count) {
            pthread_mutex_unlock(&schedule->threads_done);
        }
        thread_suspend(mach_thread_self());
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

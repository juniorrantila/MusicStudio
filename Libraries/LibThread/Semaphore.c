#include "./Semaphore.h"

#include <Basic/Error.h>
#include <Basic/Verify.h>
#include <Basic/Context.h>
#include <mach/clock_types.h>
#include <mach/semaphore.h>

#if TARGET_OS_OSX
#include <errno.h>
#include <mach/kern_return.h>
#include <mach/mach_error.h>
#include <mach/sync_policy.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#endif

#if TARGET_OS_OSX

C_API void th_sem_init(THSemaphore* sem, i32 initial_value)
{
    sem->owner = mach_task_self();
    kern_return_t rv = semaphore_create(sem->owner, &sem->event, SYNC_POLICY_FIFO, initial_value);
    VERIFY(rv == KERN_SUCCESS);
}

C_API void th_sem_deinit(THSemaphore* sem)
{
    if (sem->event) semaphore_destroy(sem->owner, sem->event);
    sem->event = 0;
}

C_API void th_sem_signal(THSemaphore* sem)
{
    if (!C_ASSERT(sem != nullptr)) return;
    if (!C_ASSERT(sem->event != 0)) return;
    semaphore_signal(sem->event);
}

C_API KError th_sem_wait_for(THSemaphore* sem, i32 milliseconds)
{
    if (!C_ASSERT(sem != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(sem->event != 0)) return kerror_unix(EINVAL);
    if (!C_ASSERT(milliseconds >= 0)) return kerror_unix(EINVAL);

    i32 seconds = milliseconds / 1000;
    milliseconds -= seconds * 1000;
    mach_timespec_t timeout = {
        .tv_sec = seconds,
        .tv_nsec = milliseconds * 1'000'000,
    };
    kern_return_t rv = semaphore_timedwait(sem->event, timeout);
    switch (rv) {
    case KERN_SUCCESS: return kerror_none;
    case KERN_OPERATION_TIMED_OUT: return kerror_unix(EAGAIN);
    case KERN_INVALID_ARGUMENT: return kerror_unix(EINVAL);
    case KERN_TERMINATED: return kerror_unix(EINVAL);
    default:
        errorf("could not wait for semaphore: %s", mach_error_string(rv));
        return kerror_unix(EINVAL);
    }
}

C_API KError th_sem_wait(THSemaphore* sem)
{
    if (!C_ASSERT(sem != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(sem->event != 0)) return kerror_unix(EINVAL);
    kern_return_t rv = KERN_ABORTED;
    while (rv == KERN_ABORTED) rv = semaphore_wait(sem->event);
    switch (rv) {
    case KERN_SUCCESS: return kerror_none;
    case KERN_OPERATION_TIMED_OUT: return kerror_unix(EAGAIN);
    case KERN_INVALID_ARGUMENT:
        errorf("could not wait for semaphore: %s", mach_error_string(rv));
        return kerror_unix(EINVAL);
    case KERN_TERMINATED:
        errorf("could not wait for semaphore: %s", mach_error_string(rv));
        return kerror_unix(EINVAL);
    default:
        errorf("could not wait for semaphore: %s", mach_error_string(rv));
        return kerror_unix(EINVAL);
    }
}

#else
#error "unsupported OS"
#endif

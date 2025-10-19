#include "./MemoryPressureMonitor.h"

#include "./Verify.h"

#include <sys/event.h>
#include <unistd.h>

#define EVFILT_MEMORYSTATUS	(-14)	/* Memorystatus events */

/*
 * data/hint fflags for EVFILT_MEMORYSTATUS, shared with userspace.
 */
#define NOTE_MEMORYSTATUS_PRESSURE_NORMAL	0x00000001	/* system memory pressure has returned to normal */
#define NOTE_MEMORYSTATUS_PRESSURE_WARN		0x00000002	/* system memory pressure has changed to the warning state */
#define NOTE_MEMORYSTATUS_PRESSURE_CRITICAL	0x00000004	/* system memory pressure has changed to the critical state */
#define NOTE_MEMORYSTATUS_LOW_SWAP		0x00000008	/* system is in a low-swap state */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN	0x00000010	/* process memory limit has hit a warning state */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL	0x00000020	/* process memory limit has hit a critical state - soft limit */
#define NOTE_MEMORYSTATUS_MSL_STATUS   0xf0000000      /* bits used to request change to process MSL status */

/*
 * data/hint fflags for EVFILT_MEMORYSTATUS, but not shared with userspace.
 */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN_ACTIVE        0x00000040      /* Used to restrict sending a warn event only once, per active limit, soft limits only */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_WARN_INACTIVE      0x00000080      /* Used to restrict sending a warn event only once, per inactive limit, soft limit only */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL_ACTIVE    0x00000100      /* Used to restrict sending a critical event only once per active limit, soft limit only */
#define NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL_INACTIVE  0x00000200      /* Used to restrict sending a critical event only once per inactive limit, soft limit only */

typedef enum vm_pressure_level {
        kVMPressureNormal   = 0,
        kVMPressureWarning  = 1,
        kVMPressureUrgent   = 2,
        kVMPressureCritical = 3,
} vm_pressure_level_t;

/*
 * Use this mask to protect the kernel private flags.
 */
#define EVFILT_MEMORYSTATUS_ALL_MASK \
	(NOTE_MEMORYSTATUS_PRESSURE_NORMAL | NOTE_MEMORYSTATUS_PRESSURE_WARN | NOTE_MEMORYSTATUS_PRESSURE_CRITICAL | NOTE_MEMORYSTATUS_LOW_SWAP | \
        NOTE_MEMORYSTATUS_PROC_LIMIT_WARN | NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL | NOTE_MEMORYSTATUS_MSL_STATUS)

C_API bool memory_pressure_monitor_init(MemoryPressureMonitor* monitor)
{
    int fd = kqueue();
    if (fd < 0) return false;

    *monitor = {
        .fd = fd,
        .pressure = MemoryPressure_Normal,
        .status_did_change = false,
    };

    // auto* queue = dispatch_queue_create("main queue", 0);
    // auto* source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0, 0
    //     |DISPATCH_MEMORYPRESSURE_NORMAL
    //     |DISPATCH_MEMORYPRESSURE_WARN
    //     |DISPATCH_MEMORYPRESSURE_CRITICAL
    //     ,
    //     queue
    // );
    // dispatch_set_context(queue, monitor);
    // dispatch_source_set_event_handler_f(source, [](void* ctx){
    //     auto* m = (MemoryPressureMonitor*)ctx;
    //     m->is_under_pressure = true;
    // });

    return monitor;
}

C_API void memory_pressure_monitor_deinit(MemoryPressureMonitor* monitor)
{
    close(monitor->fd);
}

C_API void memory_pressure_monitor_poll(MemoryPressureMonitor* monitor, struct timespec const* time)
{
    if (verify(monitor->fd >= 0).failed) return;

    struct kevent in_event = {
        .ident = (uptr)getpid(),
        .filter = EVFILT_MEMORYSTATUS,
        .flags = EV_ONESHOT | EV_ADD | EV_ENABLE,
        .fflags = NOTE_MEMORYSTATUS_PRESSURE_NORMAL
                |NOTE_MEMORYSTATUS_PRESSURE_WARN
                |NOTE_MEMORYSTATUS_PRESSURE_CRITICAL
                // |NOTE_MEMORYSTATUS_LOW_SWAP
                |NOTE_MEMORYSTATUS_PROC_LIMIT_WARN
                |NOTE_MEMORYSTATUS_PROC_LIMIT_CRITICAL
                ,
        .data = 0,
        .udata = nullptr,
    };
    monitor->status_did_change = false;

    MemoryPressure pressure = MemoryPressure_Normal;
    struct kevent out_event = {};
    int event_count = kevent(monitor->fd, &in_event, 1, &out_event, 1, time);
    if (event_count < 0) return;
    if (event_count >= 1) {
        if (out_event.flags & EV_ERROR) return;
        if (out_event.fflags & NOTE_MEMORYSTATUS_PRESSURE_CRITICAL) {
            pressure = MemoryPressure_Critical;
        }
        if (out_event.fflags & NOTE_MEMORYSTATUS_PRESSURE_WARN) {
            pressure = MemoryPressure_Warning;
        }
        if (out_event.fflags & NOTE_MEMORYSTATUS_PRESSURE_NORMAL) {
            pressure = MemoryPressure_Normal;
        }
    }
    if (pressure != monitor->pressure) {
        monitor->pressure = pressure;
        monitor->status_did_change = true;
    } else {
        monitor->status_did_change = false;
    }
}

C_API bool memory_pressure_did_change_to_normal(MemoryPressureMonitor const* monitor)
{
    if (!monitor->status_did_change)
        return false;
    return monitor->pressure == MemoryPressure_Normal;
}

C_API bool memory_pressure_did_change_to_warning(MemoryPressureMonitor const* monitor)
{
    if (!monitor->status_did_change)
        return false;
    return monitor->pressure == MemoryPressure_Warning;
}

C_API bool memory_pressure_did_change_to_critical(MemoryPressureMonitor const* monitor)
{
    if (!monitor->status_did_change)
        return false;
    return monitor->pressure == MemoryPressure_Warning;
}

#include "./Logger.h"
#include "./MessageQueue.h"

#include <Basic/Allocator.h>
#include <Basic/FileLogger.h>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define RED    "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define CYAN   "\033[1;36m"
#define NORMAL "\033[0;0m"

typedef struct THLogger THLogger;
typedef struct LogEvent {
    LoggerEvent event;
    u32 size;
    char data[];
} LogEvent;

static constexpr THMessageKind THLogEventKind_Log = TH_MESSAGE_KIND("th.log");

static void dispatch(struct Logger*, LoggerEvent);
static void logger_thread(void*);

C_API KError th_logger_init(THLogger* l, Logger* inner)
{
    KError error = kerror_none;
    if (!C_ASSERT(l != nullptr)) return kerror_unix(EINVAL);
    if (!C_ASSERT(inner != nullptr)) return kerror_unix(EINVAL);

    MEMZERO(l);
    *l = (THLogger){
        .logger = logger_init(dispatch),
        .inner = inner,
    };

    error = th_message_queue_init(&l->queue, 16 * KiB);
    if (!error.ok) return error;

    error = th_thread_init(&l->io_thread, "logger", (Context){}, l, logger_thread);
    if (!error.ok) {
        th_message_queue_deinit(&l->queue);
        return error;
    }

    return kerror_none;
}

C_API void th_logger_start(THLogger* l) { th_thread_start(&l->io_thread); }

static void dispatch(Logger* l, LoggerEvent event)
{
    THLogger* logger = FIELD_BASE(THLogger, logger, l);

    if (event.tag == LoggerEventTag_Fatal) {
        __rtsan_disable();
        FileLogger file_logger = file_logger_init(context()->thread_name, stderr);
        file_logger.logger.dispatch(&file_logger.logger, event);
        abort();
        __rtsan_enable();
    }

    LogEvent* e = tpush(sizeof(LogEvent) + event.message_size, alignof(LogEvent));
    if (!e) {
        __rtsan_disable();
        FileLogger file_logger = file_logger_init(context()->thread_name, stderr);
        log_warning(&file_logger.logger, "log message too big for deferred dispatch, logging directly (seq: %.2u)", event.seq % 100);
        file_logger.logger.dispatch(&file_logger.logger, event);
        __rtsan_enable();
        return;
    }
    *e = (LogEvent){
        .event = event,
        .size = event.message_size,
    };
    e->event.message = nullptr;
    memcpy(e->data, event.message, event.message_size);
    KError err = th_message_send(&logger->queue, THLogEventKind_Log, e, sizeof(*e) + e->size, alignof(LogEvent));
    if (!err.ok) {
        __rtsan_disable();
        FileLogger file_logger = file_logger_init(context()->thread_name, stderr);
        log_warning(&file_logger.logger, "failed to dispatch deferred log message(%s), logging directly (seq: %.2u)", kerror_strerror(err), event.seq % 100);
        file_logger.logger.dispatch(&file_logger.logger, event);
        __rtsan_enable();
        return;
    }
}

static void handle_event(THLogger*, LogEvent*);

static void logger_thread(void* user)
{
    THLogger* logger = (THLogger*)user;

    for (;;) {
        reset_temporary_arena();

        THMessage message = {0};
        KError error = th_message_wait(&logger->queue, &message);
        if (!error.ok) {
            warnf("could not get message: %s", kerror_strerror(error));
            continue;
        }

        if (!C_ASSERT(message.kind.tag == THLogEventKind_Log.tag)) {
            errorf("expected '%.8s', got '%.8s'", THLogEventKind_Log.name, message.kind.name);
            continue;
        }
        if (!C_ASSERT(message.align == alignof(LogEvent))) continue;
        if (!C_ASSERT(message.data != nullptr)) continue;
        LogEvent* event = (LogEvent*)message.data;
        if (!C_ASSERT(message.size == sizeof(*event) + event->size)) continue;

        handle_event(logger, event);
    }

    UNREACHABLE();
}

static void handle_event(THLogger* logger, LogEvent* event)
{
    if (!C_ASSERT(logger != nullptr)) return;
    if (!C_ASSERT(event != nullptr)) return;
    if (!C_ASSERT(logger->inner)) return;
    if (!C_ASSERT(logger->inner->dispatch)) return;

    event->event.message = event->data;
    event->event.message_size = event->size;
    logger->inner->dispatch(logger->inner, event->event);
}

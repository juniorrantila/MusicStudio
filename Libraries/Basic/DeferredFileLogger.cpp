#include "./DeferredFileLogger.h"

#include "./Logger.h"
#include "./FileLogger.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void dispatch(struct Logger*, LoggerEvent);

#define RED    "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define CYAN   "\033[1;36m"
#define NORMAL "\033[0;0m"

C_API DeferredFileLogger deferred_file_logger_init(c_string name, Mailbox* mailbox, FILE* file)
{
    return (DeferredFileLogger){
        .logger = logger_init(dispatch),
        .mailbox = mailbox,
        .file = file,
        .name = name,
        .is_tty = isatty(fileno(file)) == 1,
        .arena_buffer = {},
    };
}

static c_string severity(LoggerEventTag tag)
{
    switch (tag) {
    case LoggerEventTag_Format: return "";
    case LoggerEventTag_Debug: return "DBUG";
    case LoggerEventTag_Info: return "INFO";
    case LoggerEventTag_Warning: return "WARN";
    case LoggerEventTag_Error: return "ERRR";
    case LoggerEventTag_Fatal: return "FATL";
    }
}

static c_string reset(bool is_tty)
{
    if (!is_tty) return "";
    return NORMAL;
}

static c_string color(LoggerEventTag tag, bool is_tty)
{
    if (!is_tty) return "";
    switch (tag) {
    case LoggerEventTag_Format: return "";
    case LoggerEventTag_Debug: return CYAN;
    case LoggerEventTag_Info: return BLUE;
    case LoggerEventTag_Warning: return YELLOW;
    case LoggerEventTag_Error:
    case LoggerEventTag_Fatal: return RED;
    }
}

static void dispatch(struct Logger* l, LoggerEvent event)
{
    DeferredFileLogger* logger = FIELD_BASE(DeferredFileLogger, logger, l);
    pid_t pid = getpid();

    if (event.tag == LoggerEventTag_Fatal) {
        __rtsan_disable();
        static FileLogger file_logger = file_logger_init(logger->file);
        file_logger.name = logger->name;
        file_logger.logger.dispatch(&file_logger.logger, event);
        abort();
        __rtsan_enable();
    }

    if (event.message_size > deferred_log_event_message_size_max) {
        __rtsan_disable();
        static FileLogger file_logger = file_logger_init(logger->file);
        file_logger.name = logger->name;
        file_logger.logger.warning("log message too big for deferred dispatch, logging directly (seq: %.2u)", event.seq % 100);
        file_logger.logger.dispatch(&file_logger.logger, event);
        __rtsan_enable();
        return;
    }

    static_assert(sizeof(DeferredLogEvent::message_size) == 1);
    auto e = (DeferredLogEvent){
        .logger = logger,
        .severity = event.tag,
        .pid = pid,
        .seq = event.seq,
        .message_size = (u8)event.message_size,
        .message = {},
    };
    memcpy(e.message, event.message, event.message_size);
    if (!logger->mailbox->writer()->post(e).ok) {
        __rtsan_disable();
        static FileLogger file_logger = file_logger_init(logger->file);
        file_logger.name = logger->name;
        file_logger.logger.warning("failed to dispatch deferred log message, logging directly (seq: %.2u)", event.seq % 100);
        file_logger.logger.dispatch(&file_logger.logger, event);
        __rtsan_enable();
    }
}

void DeferredFileLogger::handle_event(DeferredLogEvent const* event) { return deferred_file_logger_handle_event(this, event); }
void deferred_file_logger_handle_event(DeferredFileLogger* logger, DeferredLogEvent const* event)
{
    if (event->severity == LoggerEventTag_Format) {
        (void)fwrite(event->message, event->message_size, 1, logger->file);
        (void)fflush(logger->file);
        return;
    }

    bool is_tty = logger->is_tty;
    auto tag = event->severity;
    if (logger->name) {
        (void)fprintf(logger->file, "%s%s|%s%.2u%s|%s%u%s|%s%16s%s|%s %.*s\n",
            color(tag, is_tty), // 1
            severity(tag),
            reset(is_tty),
            event->seq % 100,
            color(tag, is_tty),
            reset(is_tty),
            event->pid,
            color(tag, is_tty),
            reset(is_tty),
            logger->name,
            color(tag, is_tty),
            reset(is_tty),
            event->message_size,
            event->message
        );
    } else {
                                   // a b  c   d e    f g    hi
        (void)fprintf(logger->file, "%s%s|%s%.2u%s|%u%s: %.*s\n",
            color(tag, is_tty),  // a
            severity(tag),       // b
            reset(is_tty),       // c
            event->seq % 100,    // d
            color(tag, is_tty),  // e
            event->pid,          // f
            reset(is_tty),       // g
            event->message_size, // h
            event->message       // i
        );
    }

    (void)fflush(logger->file);

    if (tag == LoggerEventTag_Fatal) abort();
}

#pragma once
#include "./Logger.h"
#include "./Mailbox.h"

#include <stdio.h>
#include <pthread.h>

constexpr u64 deferred_log_event_message_size_max = 227;
typedef struct DeferredFileLogger DeferredFileLogger;
typedef struct {
    DeferredFileLogger* logger;
    LoggerEventTag severity;
    i32 pid;
    u32 seq;
    u8 message_size;
    char message[deferred_log_event_message_size_max];
} DeferredLogEvent;
static_assert(sizeof(DeferredLogEvent) <= message_size_max);

typedef struct DeferredFileLogger {
    Logger logger;
    Allocator* temporary_arena;
    Mailbox* mailbox;
    FILE* file;
    c_string name;
    bool is_tty;

#ifdef __cplusplus
    void handle_event(DeferredLogEvent const*);
#endif
} DeferredFileLogger;

C_API DeferredFileLogger make_deferred_file_logger(Allocator* temporary_arena, Mailbox* mailbox, FILE* file);

C_API void deferred_file_logger_handle_event(DeferredFileLogger*, DeferredLogEvent const*);

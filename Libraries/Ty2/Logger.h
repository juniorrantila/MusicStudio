#pragma once
#include "./Base.h"
#include "./Allocator.h"
#include "./FixedArena.h"
#include "./Bits.h"

#include <stdarg.h>

typedef enum LoggerEventTag {
    LoggerEventTag_Debug,
    LoggerEventTag_Info,
    LoggerEventTag_Warning,
    LoggerEventTag_Error,
    LoggerEventTag_Fatal,
} LoggerEventTag;

typedef struct LoggerEvent {
    char const* message;
    usize message_size;
    LoggerEventTag tag;
    u32 seq;
} LoggerEvent;

typedef struct Logger {
    void (*dispatch)(struct Logger*, LoggerEvent);
    char arena_buffer[1 * KiB];

#ifdef __cplusplus

    [[gnu::format(printf, 2, 3)]]
    void debug(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    void vdebug(c_string, va_list);

    [[gnu::format(printf, 2, 3)]]
    void info(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    void vinfo(c_string, va_list);

    [[gnu::format(printf, 2, 3)]]
    void warning(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    void vwarning(c_string, va_list);

    [[gnu::format(printf, 2, 3)]]
    void error(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    void verror(c_string, va_list);

    [[noreturn, gnu::format(printf, 2, 3)]]
    void fatal(c_string, ...);
    [[noreturn, gnu::format(printf, 2, 0)]]
    void vfatal(c_string, va_list);
#endif
} Logger;

C_API inline Logger logger_init(void(*dispatch)(struct Logger*, LoggerEvent))
{
    return (Logger) {
        .dispatch = dispatch,
        .arena_buffer = {},
    };
}

__attribute__((format(printf, 2, 3)))
C_API void log_debug(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_debug(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_info(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_info(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_warning(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_warning(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_error(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_error(Logger*, c_string, va_list);

__attribute__((noreturn, format(printf, 2, 3)))
C_API void log_fatal(Logger*, c_string, ...);

__attribute__((noreturn, format(printf, 2, 0)))
C_API void vlog_fatal(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_debug_if(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_debug_if(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_info_if(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_info_if(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_warning_if(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_warning_if(Logger*, c_string, va_list);

__attribute__((format(printf, 2, 3)))
C_API void log_error_if(Logger*, c_string, ...);
__attribute__((format(printf, 2, 0)))
C_API void vlog_error_if(Logger*, c_string, va_list);

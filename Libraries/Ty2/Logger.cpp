#include "./Logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

struct Format {
    char* buf;
    usize buf_len;
    bool used_allocator;
};

[[gnu::format(printf, 2, 0)]]
static Format format_resolve(Allocator*, c_string fmt, va_list args);

[[gnu::format(printf, 3, 0)]]
static void log_generic(Logger* l, LoggerEventTag tag, c_string fmt, va_list args)
{
    Format format = format_resolve(l->temporary_arena, fmt, args);
    if (!format.buf) return;

    l->dispatch(l, (LoggerEvent){
        .message = format.buf,
        .message_size = format.buf_len,
        .tag = tag,
    });

    if (format.used_allocator) {
        memfree(l->temporary_arena, format.buf, format.buf_len, 1);
    } else {
        free(format.buf);
    }
}

C_API void vlog_debug(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Debug, fmt, args); }
C_API void vlog_info(Logger* l, c_string fmt, va_list args)  { log_generic(l, LoggerEventTag_Info, fmt, args); }
C_API void vlog_warning(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Warning, fmt, args); }
C_API void vlog_error(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Error, fmt, args); }
C_API void vlog_fatal(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Fatal, fmt, args); abort(); }

C_API void log_debug(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_debug(l, fmt, args);
    va_end(args);
}

C_API void log_info(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_info(l, fmt, args);
    va_end(args);
}

C_API void log_warning(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_warning(l, fmt, args);
    va_end(args);
}

C_API void log_error(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_error(l, fmt, args);
    va_end(args);
}

C_API void log_fatal(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_fatal(l, fmt, args);
    va_end(args);
}

void Logger::debug(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vdebug(fmt, args);
    va_end(args);
}
void Logger::vdebug(c_string fmt, va_list args) { vlog_debug(this, fmt, args); }

void Logger::info(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vinfo(fmt, args);
    va_end(args);
}
void Logger::vinfo(c_string fmt, va_list args) { vlog_info(this, fmt, args); }

void Logger::warning(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vinfo(fmt, args);
    va_end(args);
}
void Logger::vwarning(c_string fmt, va_list args) { vlog_warning(this, fmt, args); }

void Logger::error(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verror(fmt, args);
    va_end(args);
}
void Logger::verror(c_string fmt, va_list args) { vlog_error(this, fmt, args); }

void Logger::fatal(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfatal(fmt, args);
    va_end(args);
}
void Logger::vfatal(c_string fmt, va_list args) { vlog_fatal(this, fmt, args); }

static Format format_resolve(Allocator* a, c_string fmt, va_list args)
{
    va_list args2;
    va_copy(args2, args);


    int len = vsnprintf(0, 0, fmt, args);
    if (len < 0) {
        va_end(args2);
        (void)fprintf(stderr, "ERROR: could not resolve format length\n");
        return {
            .buf = 0,
            .buf_len = 0,
            .used_allocator = false,
        };
    }
    len += 1;

    char* buf = (char*)memalloc(a, len, 1);
    if (!buf) {
        buf = (char*)calloc(len, 1);
        if (!buf) {
            (void)fprintf(stderr, "ERROR: could not allocate memory for message\n");
            (void)vfprintf(stderr, fmt, args2);
            va_end(args2);
            return {
                .buf = 0,
                .buf_len = 0,
                .used_allocator = false,
            };
        }
        va_end(args2);
        return {
            .buf = buf,
            .buf_len = (usize)len,
            .used_allocator = false,
        };
    }

    (void)vsnprintf(buf, len, fmt, args2);
    va_end(args2);
    return {
        .buf = buf,
        .buf_len = (usize)len,
        .used_allocator = true,
    };
}

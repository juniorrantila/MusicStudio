#include "./Logger.h"
#include "./Verify.h"

#include <stdarg.h>
#include <stb/sprintf.h>

struct Format {
    char* buf;
    usize buf_len;
};

[[gnu::format(printf, 2, 0)]]
static Format format_resolve(Allocator*, c_string fmt, va_list args);

[[gnu::format(printf, 3, 0)]]
static void log_generic(Logger* l, LoggerEventTag tag, c_string fmt, va_list args)
{
    static _Atomic u32 seq;

    auto arena = fixed_arena_init(l->arena_buffer, sizeof(l->arena_buffer));

    Format format = format_resolve(&arena.allocator, fmt, args);
    if (!format.buf) return;

    l->dispatch(l, (LoggerEvent){
        .message = format.buf,
        .message_size = format.buf_len,
        .tag = tag,
        .seq = seq++,
    });
}

C_API void vlog_format(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Format, fmt, args); }
C_API void vlog_debug(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Debug, fmt, args); }
C_API void vlog_info(Logger* l, c_string fmt, va_list args)  { log_generic(l, LoggerEventTag_Info, fmt, args); }
C_API void vlog_warning(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Warning, fmt, args); }
C_API void vlog_error(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Error, fmt, args); }
C_API void vlog_fatal(Logger* l, c_string fmt, va_list args) { log_generic(l, LoggerEventTag_Fatal, fmt, args); __builtin_abort(); }

C_API void log_format(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_format(l, fmt, args);
    va_end(args);
}

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

void Logger::vformat(c_string fmt, va_list args) { vlog_format(this, fmt, args); }
void Logger::format(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vformat(fmt, args);
    va_end(args);
}

void Logger::vdebug(c_string fmt, va_list args) { vlog_debug(this, fmt, args); }
void Logger::debug(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vdebug(fmt, args);
    va_end(args);
}

void Logger::vinfo(c_string fmt, va_list args) { vlog_info(this, fmt, args); }
void Logger::info(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vinfo(fmt, args);
    va_end(args);
}

void Logger::vwarning(c_string fmt, va_list args) { vlog_warning(this, fmt, args); }
void Logger::warning(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vwarning(fmt, args);
    va_end(args);
}

void Logger::verror(c_string fmt, va_list args) { vlog_error(this, fmt, args); }
void Logger::error(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verror(fmt, args);
    va_end(args);
}

void Logger::vfatal(c_string fmt, va_list args) { vlog_fatal(this, fmt, args); }
void Logger::fatal(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfatal(fmt, args);
    va_end(args);
}


C_API void log_debug_if(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vlog_debug_if(l, fmt, args);

    va_end(args);
}
__attribute__((format(printf, 2, 0)))
C_API void vlog_debug_if(Logger* l, c_string fmt, va_list args)
{
    if (!l) return;
    vlog_debug(l, fmt, args);
}

C_API void log_info_if(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vlog_info_if(l, fmt, args);

    va_end(args);
}

C_API void vlog_info_if(Logger* l, c_string fmt, va_list args)
{
    if (!l) return;
    vlog_info(l, fmt, args);
}

C_API void log_warning_if(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vlog_warning_if(l, fmt, args);

    va_end(args);
}

C_API void vlog_warning_if(Logger* l, c_string fmt, va_list args)
{
    if (!l) return;
    vlog_warning(l, fmt, args);
}

C_API void log_error_if(Logger* l, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vlog_error_if(l, fmt, args);

    va_end(args);
}

C_API void vlog_error_if(Logger* l, c_string fmt, va_list args)
{
    if (!l) return;
    vlog_error(l, fmt, args);
}

static Format format_resolve(Allocator* a, c_string fmt, va_list args)
{
    va_list args2;
    va_copy(args2, args);

    int len = stb_vsnprintf(0, 0, fmt, args);
    VERIFY(len >= 0);

    char* buf = (char*)memalloc(a, len + 1, 1);
    VERIFY(buf != nullptr);
    __builtin_memset(buf, 0, len);

    int len2 = stb_vsnprintf(buf, len + 1, fmt, args2);
    VERIFY(len == len2);

    va_end(args2);
    return {
        .buf = buf,
        .buf_len = (usize)len,
    };
}

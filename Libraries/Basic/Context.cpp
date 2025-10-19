#include "./Context.h"

#include "./Bits.h"
#include "./FileLogger.h"
#include "./FixedArena.h"
#include "./Allocator.h"
#include "./Error.h"
#include "./StringSlice.h"
#include "./Verify.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <stb/sprintf.h>

#ifndef NDEBUG
#include <libgen.h>
#endif

extern thread_local Context* g_context;
extern thread_local bool g_context_is_initialized;

thread_local Context* g_context;
thread_local bool g_context_is_initialized = 0;

C_API void push_context(Context* next)
{
    VERIFY(g_context_is_initialized);
    VERIFY(next != nullptr);
    next->previous = context();
    if (!next->log) next->log = context()->log;
    if (!next->temp_arena) next->temp_arena = context()->temp_arena;
    set_context(next);
}

C_API void pop_context(void)
{
    VERIFY(g_context_is_initialized);
    set_context(context()->previous);
}

C_API void set_context(Context* context)
{
    g_context = context;
    g_context_is_initialized = true;
}

C_API void init_context(Context* context)
{
    DEBUG_ASSERT(!g_context_is_initialized);
    set_context(context);
}

C_API Context* context(void)
{
#ifndef NDEBUG
    if (!g_context_is_initialized) {
        static c_string program_name;
        static char buf[PATH_MAX];
        if (!program_name) {
            basename_r(getenv("_"), buf);
            program_name = buf;
        }
        init_default_context(program_name);
        warnf("context was not initialized");
    }
#endif
    VERIFY(g_context_is_initialized);
    return g_context;
}


void init_default_context(c_string thread_name)
{
    DEBUG_ASSERT(!g_context_is_initialized);
    static thread_local Context context;
    init_default_context_into(thread_name, &context);
    set_context(&context);
}

void init_default_context_into(c_string thread_name, Context* out)
{
    VERIFY(thread_name != nullptr);

    pthread_setname_np(thread_name);

    static thread_local struct {
        FileLogger log;
        u8 arena_buffer[16 * KiB];
        FixedArena arena;
    } context;

    context.log = file_logger_init(thread_name, stderr);
    context.arena = fixed_arena_init(context.arena_buffer, sizeof(context.arena_buffer));
    *out = (Context){
        .log = &context.log.logger,
        .temp_arena = &context.arena,
    };
}

C_API c_string tprint(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    c_string result = tvprint(fmt, args);
    va_end(args);
    return result;
}

C_API c_string tvprint(c_string fmt, va_list args)
{
    int len = stb_vsnprintf(nullptr, 0, fmt, args);
    if (len < 0) return nullptr;
    if (len == 0) return "";
    char* buf = (char*)tpush(len + 1, 1);
    memzero(buf, len + 1);
    if (!buf) return nullptr;
    int len2 = stb_vsnprintf(buf, len + 1, fmt, args);
    VERIFY(len == len2);
    return buf;
}

C_API KError tprints(StringSlice* out, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    KError result = tvprints(out, fmt, args);
    va_end(args);
    return result;
}

C_API KError tvprints(StringSlice* out, c_string fmt, va_list args)
{
    c_string result = tvprint(fmt, args);
    if (!result)
        return kerror_unix(ENOMEM);
    *out = sv_from_c_string(result);
    return kerror_none;
}

C_API Allocator* temporary_arena(void)
{
    return &context()->temp_arena->allocator;
}

C_API void* tpush(u64 size, u64 align)
{
    return context()->temp_arena->push(size, align);
}

C_API void* tclone(void const* data, u64 size, u64 align)
{
    return memclone(&context()->temp_arena->allocator, data, size, align);
}

C_API void drain_temporary_arena(void)
{
#ifndef NDEBUG
    {
        u64 used = tbytes_used();
        if (!used) return;
            static thread_local u32 report;
            if (report++ % 500 == 0) {
                u64 left = tbytes_left();
                debugf("arena used: %lu left: %lu", used, left);
            }
    }
#endif
    context()->temp_arena->drain();
}

C_API u64 tbytes_used(void)
{
    return context()->temp_arena->bytes_used();
}

C_API u64 tbytes_left(void)
{
    return context()->temp_arena->bytes_left();
}

C_API void print(c_string fmt, ...)   { va_list args; va_start(args, fmt); vprint(fmt, args);  va_end(args); }
C_API void debugf_(c_string fmt, ...) { va_list args; va_start(args, fmt); vdebugf(fmt, args); va_end(args); }
C_API void infof_(c_string fmt, ...)  { va_list args; va_start(args, fmt); vinfof(fmt, args);  va_end(args); }
C_API void warnf_(c_string fmt, ...)  { va_list args; va_start(args, fmt); vwarnf(fmt, args);  va_end(args); }
C_API void errorf_(c_string fmt, ...) { va_list args; va_start(args, fmt); verrorf(fmt, args); va_end(args); }
C_API void fatalf_(c_string fmt, ...) { va_list args; va_start(args, fmt); vfatalf(fmt, args); va_end(args); }

C_API void vprint(c_string fmt, va_list args) { context()->log->vformat(fmt, args); }
C_API void vdebugf(c_string fmt, va_list args) { context()->log->vdebug(fmt, args); }
C_API void vinfof(c_string fmt, va_list args) { context()->log->vinfo(fmt, args); }
C_API void vwarnf(c_string fmt, va_list args) { context()->log->vwarning(fmt, args); }
C_API void verrorf(c_string fmt, va_list args) { context()->log->verror(fmt, args); }
C_API void vfatalf(c_string fmt, va_list args) { context()->log->vfatal(fmt, args); }

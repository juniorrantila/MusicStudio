#include "./Thread.h"

#include <Basic/Verify.h>
#include <Basic/FileLogger.h>
#include <Basic/FixedArena.h>
#include <Basic/Context.h>

#if PLATFORM_POSIX

static void* entry_proc(void*);

C_API KError th_thread_init(THThread* thread, c_string name, Context starting_context, void* data, void(*proc)(void*))
{
    MEMZERO(thread);
    thread->name = name;
    thread->id = kthread_id_next();
    thread->starting_context = starting_context;
    thread->user = data;
    thread->proc = proc;
    th_sem_init(&thread->suspended, 0);

    int err = pthread_create(&thread->thread_handle, nullptr, entry_proc, thread);
    if (err != 0) {
        th_sem_deinit(&thread->suspended);
        return kerror_unix(err);
    }

    return kerror_none;
}

C_API void th_thread_start(THThread* thread) { th_sem_signal(&thread->suspended); }

static void* entry_proc(void* user)
{
    THThread* thread = (THThread*)user;
    KError error = th_sem_wait(&thread->suspended);
    if (!C_ASSERT(error.ok)) return nullptr;

    FileLogger default_log = file_logger_init(thread->name, stderr);
    u8 arena_buffer[16 * KiB];
    FixedArena arena = fixed_arena_init(arena_buffer, sizeof(arena_buffer));

    Context* context = &thread->starting_context;

    context->thread_id = thread->id;
    if (!context->log) context->log = &default_log.logger;
    if (!context->temp_arena) context->temp_arena = &arena;

    context->thread_id = thread->id;

#if PLATFORM_OSX
    pthread_setname_np(thread->name);
#endif

    init_context(context);
    thread->proc(thread->user);

    return nullptr;
}

#else
#error "unsupported platform"
#endif

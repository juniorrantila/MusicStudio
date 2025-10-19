#include "./Thread.h"

#include <pthread.h>

C_API [[nodiscard]] bool core_thread_create(void* user, void(*callback)(void*))
{
    pthread_t thread;
    int res = pthread_create(&thread, nullptr, (void*(*)(void*))(void*)callback, user);
    if (res != 0) return false;
    return true;
}

C_API void core_thread_set_name(c_string name)
{
    pthread_setname_np(name);
}

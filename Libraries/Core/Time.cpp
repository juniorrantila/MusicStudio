#include "./Time.h"

#include <time.h>

namespace Core {

f64 time(void)
{
    struct timespec spec {};
    (void)clock_gettime(CLOCK_REALTIME, &spec);
    return ((f64)spec.tv_sec) + (((f64)spec.tv_nsec) / 1.0e9);
}

}

C_API f64 core_time_now()
{
    return Core::time();
}

C_API f64 core_time_since_start()
{
    static f64 start;
    if (start == 0.0) {
        start = core_time_now();
    }
    return core_time_now() - start;
}

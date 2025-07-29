#include "./Time.h"

#include <float.h>
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
    f64 now = core_time_now();
    f64 time_since_start = now - start;

    // Returning epsilon here since calling core_time_now() twice might yield
    // a difference of zero, which is best to avoid for parts of the code base
    // which assume that a value of zero means uninitialized.
    if (time_since_start < DBL_EPSILON)
        return DBL_EPSILON;
    return time_since_start;
}

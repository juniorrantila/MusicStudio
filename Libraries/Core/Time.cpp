#include "./Time.h"

#include <time.h>

namespace Core {

f64 time(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return ((f64)spec.tv_sec) + (spec.tv_nsec / 1.0e9);
}

}

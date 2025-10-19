#include "./Util.h"
#include "./Arena.h"

#include <string.h>

static Arena temporary_arena = {};

char *temp_strdup(c_string s)
{
    size_t n = strlen(s);
    char *ds = (char*)arena_alloc(&temporary_arena, n + 1);
    memcpy(ds, s, n);
    ds[n] = '\0';
    return ds;
}

void temp_reset(void)
{
    arena_reset(&temporary_arena);
}

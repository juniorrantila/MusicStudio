#pragma once
#include <Ty/Base.h>
#include <assert.h>
#include <errno.h>

typedef int Errno;

#define SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

#define return_defer(value) do { result = (value); goto defer; } while (0)

#define UNIMPLEMENTED(...)                                                      \
    do {                                                                        \
        printf("%s:%d: UNIMPLEMENTED: %s \n", __FILE__, __LINE__, __VA_ARGS__); \
        exit(1);                                                                \
    } while(0)
#define UNREACHABLE(...)                                                      \
    do {                                                                      \
        printf("%s:%d: UNREACHABLE: %s \n", __FILE__, __LINE__, __VA_ARGS__); \
        exit(1);                                                              \
    } while(0)
#define UNUSED(x) (void)(x)

#define DA_INIT_CAP 256

#define da_last(da) (assert((da)->count > 0), (da)->items[(da)->count - 1])

#define da_move(dst, src)                \
    do {                                 \
       free((dst)->items);               \
       (dst)->items = (src).items;       \
       (dst)->count = (src).count;       \
       (dst)->capacity = (src).capacity; \
    } while (0)

#define da_append(da, item)                                                          \
    do {                                                                             \
        if ((da)->count >= (da)->capacity) {                                         \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = (__typeof((da)->items))realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                            \
                                                                                     \
        (da)->items[(da)->count++] = (item);                                         \
    } while (0)

#define da_append_many(da, new_items, new_items_count)                                      \
    do {                                                                                    \
        if ((da)->count + new_items_count > (da)->capacity) {                               \
            if ((da)->capacity == 0) {                                                      \
                (da)->capacity = DA_INIT_CAP;                                               \
            }                                                                               \
            while ((da)->count + new_items_count > (da)->capacity) {                        \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = (__typeof((da)->items))realloc((da)->items, (da)->capacity*sizeof(*(da)->items));        \
            assert((da)->items != NULL && "Buy more RAM lol");                              \
        }                                                                                   \
        memcpy((da)->items + (da)->count, new_items, new_items_count*sizeof(*(da)->items)); \
        (da)->count += new_items_count;                                                     \
    } while (0)

char *temp_strdup(c_string s);
void temp_reset(void);

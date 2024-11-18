#pragma once

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define BIT_CAST(T, ...) __builtin_bit_cast(T, __VA_ARGS__)


#define VEC_PUSH(allocator, vec, ...)                                                                                   \
    ({                                                                                                                  \
        int res = 0;                                                                                                    \
        if ((vec)->count >= (vec)->capacity) {                                                                          \
            usize new_capacity = (vec)->capacity * 2;                                                                   \
            if (new_capacity < 16) {                                                                                    \
                new_capacity = 16;                                                                                      \
            }                                                                                                           \
            __typeof((vec)->items) items = allocator_alloc_many((allocator), __typeof(*(vec)->items), new_capacity);    \
            if (items == 0) {                                                                                           \
                res = -1;                                                                                               \
            } else {                                                                                                    \
                if ((vec)->items != 0) {                                                                                \
                    memcpy(items, (vec)->items, (vec)->count * sizeof(*(vec)->items));                                  \
                }                                                                                                       \
                (vec)->items = items;                                                                                   \
            }                                                                                                           \
        }                                                                                                               \
        if (res == 0) {                                                                                                 \
            (vec)->items[(vec)->count++] = (__VA_ARGS__);                                                               \
        }                                                                                                               \
        res;                                                                                                            \
    })

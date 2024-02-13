#pragma once

#define VERIFY(__expression)                                  \
    do {                                                      \
        if (!(__expression)) {                                \
            __builtin_printf(                                 \
                "VERIFY: %s: \"%s\" failed [%s:%d]\n",        \
                __PRETTY_FUNCTION__, #__expression, __FILE__, \
                __LINE__);                                    \
            __builtin_trap();                                 \
        }                                                     \
    } while (0)

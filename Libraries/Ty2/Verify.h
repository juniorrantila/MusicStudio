#pragma once

#ifndef VERIFY
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
#endif

#ifndef VERIFYS
#define VERIFYS(__expression, message)                        \
    do {                                                      \
        if (!(__expression)) {                                \
            __builtin_printf(                                 \
                "VERIFY: %s: \"%s\" failed (%s) [%s:%d]\n",   \
                __PRETTY_FUNCTION__, #__expression, message, __FILE__, \
                __LINE__);                                    \
            __builtin_trap();                                 \
        }                                                     \
    } while (0)
#endif

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED()                     \
    do {                                    \
        __builtin_printf(                   \
            "UNIMPLEMENTED: %s [%s:%d]\n",  \
            __PRETTY_FUNCTION__, __FILE__,  \
            __LINE__);                      \
        __builtin_trap();                   \
    } while (0)
#endif

#ifndef UNREACHABLE
#define UNREACHABLE()                       \
    do {                                    \
        __builtin_printf(                   \
            "UNREACHABLE: %s [%s:%d]\n",    \
            __PRETTY_FUNCTION__, __FILE__,  \
            __LINE__);                      \
        __builtin_trap();                   \
    } while (0)
#endif

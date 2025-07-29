#pragma once

#ifndef VERIFY_RED
#define VERIFY_RED    "\033[1;31m"
#define VERIFY_YELLOW "\033[1;33m"
#define VERIFY_BLUE   "\033[1;34m"
#define VERIFY_CYAN   "\033[0;36m"
#define VERIFY_NORMAL "\033[0;0m"
#endif

#ifndef verify
typedef struct [[nodiscard]] { bool failed; } VerifyFail;
#define verify(__expression)                \
    ({                                      \
        bool __result = !(__expression);    \
        if (__result) __builtin_printf(VERIFY_RED "VERIFY" VERIFY_NORMAL ": %s: \"" VERIFY_CYAN "%s" VERIFY_NORMAL "\" " VERIFY_RED "failed" VERIFY_NORMAL " [%s:%d]\n", __FUNCTION__, #__expression, __FILE__, __LINE__);  \
        (VerifyFail){__result};             \
    })

#define verifys(__expression, message)      \
    ({                                      \
        bool __result = !(__expression);    \
        if (__result) __builtin_printf(VERIFY_RED "VERIFY" VERIFY_NORMAL ": %s: \"" VERIFY_CYAN "%s" VERIFY_NORMAL "\" " VERIFY_RED "failed" VERIFY_NORMAL " (" message ") [%s:%d]\n", __FUNCTION__, #__expression, __FILE__, __LINE__); \
        (VerifyFail){__result};             \
    })
#endif

#ifndef VERIFY
#define VERIFY(__expression)                            \
    do {                                                \
        if (!(__expression)) { \
            __builtin_printf(VERIFY_RED "VERIFY" VERIFY_NORMAL ": %s: \"" VERIFY_CYAN "%s" VERIFY_NORMAL "\" " VERIFY_RED "failed" VERIFY_NORMAL " [%s:%d]\n", __FUNCTION__, #__expression, __FILE__, __LINE__);  \
            __builtin_trap();                           \
        }                                               \
    } while (0)
#endif


#ifndef VERIFYS
#define VERIFYS(__expression, message)                  \
    do {                                                \
        if (!(__expression)) {                          \
            __builtin_printf(VERIFY_RED "VERIFY" VERIFY_NORMAL ": %s: \"" VERIFY_CYAN "%s" VERIFY_NORMAL "\" " VERIFY_RED "failed" VERIFY_NORMAL " (" message ") [%s:%d]\n", __FUNCTION__, #__expression, __FILE__, __LINE__); \
            __builtin_trap();                           \
        }                                               \
    } while (0)
#endif

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED()                                                 \
    do {                                                                \
        __builtin_printf(                                               \
            VERIFY_RED "UNIMPLEMENTED" VERIFY_NORMAL ": %s [%s:%d]\n",  \
            __PRETTY_FUNCTION__, __FILE__,                              \
            __LINE__);                                                  \
        __builtin_trap();                                               \
    } while (0)
#endif

#ifndef UNREACHABLE
#define UNREACHABLE()                                                   \
    do {                                                                \
        __builtin_printf(                                               \
            VERIFY_RED "UNREACHABLE" VERIFY_NORMAL ": %s [%s:%d]\n",    \
            __PRETTY_FUNCTION__, __FILE__,                              \
            __LINE__);                                                  \
        __builtin_trap();                                               \
    } while (0)
#endif

#ifndef panic
#define panic()                                                         \
    do {                                                                \
        __builtin_printf(                                               \
            VERIFY_RED "PANIC" VERIFY_NORMAL ": %s [%s:%d]\n",          \
            __PRETTY_FUNCTION__, __FILE__,                              \
            __LINE__);                                                  \
        __builtin_trap();                                               \
    } while (0)
#endif

#ifndef panics
#define panics(message)                                                                 \
    do {                                                                                \
        __builtin_printf(                                                               \
            VERIFY_RED "PANIC" VERIFY_NORMAL ": %s \"" message "\" [%s:%d]\n",          \
            __PRETTY_FUNCTION__, __FILE__,                                              \
            __LINE__);                                                                  \
        __builtin_trap();                                                               \
    } while (0)
#endif

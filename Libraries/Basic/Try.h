#pragma once

#ifdef __cplusplus

#define TRY(expr)                                \
    ({                                           \
        decltype(auto) _result = (expr);         \
        if (!_result.has_value()) [[unlikely]] { \
            return _result.release_error();      \
        }                                        \
        _result.release_value();                 \
    })

#define MUST(expr)                               \
    ({                                           \
        decltype(auto) _result = (expr);         \
        if (!_result.has_value()) [[unlikely]] { \
            __builtin_abort();                   \
        }                                        \
        _result.release_value();                 \
    })

#else

#define TRY(expr)                           \
    ({                                      \
        __typeof(expr) _result = (expr);    \
        if (!_result.ok) {                  \
            return _result;                 \
        }                                   \
    })

#define MUST(expr)                          \
    ({                                      \
        __typeof(expr) _result = (expr);    \
        VERIFY(_result.ok);                 \
    })

#endif

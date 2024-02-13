#pragma once

#define ASSERT(__expression)                                                          \
    do {                                                                              \
        if (!(__expression)) {                                                        \
            return Error::from_string_literal("ASSERT \'" #__expression "\' failed"); \
        }                                                                             \
    } while (0)

#define NSASSERT(__expression)                                                           \
    do {                                                                                 \
        if (!(__expression)) {                                                           \
            @throw [NSException exceptionWithName:@"assert"                              \
                                           reason:@"ASSERT \'" #__expression "\' failed" \
                                         userInfo:nil];                                  \
            ;                                                                            \
        }                                                                                \
    } while (0)

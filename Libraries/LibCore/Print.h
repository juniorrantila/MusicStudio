#pragma once
#ifdef stderr
#pragma push_macro("stderr")
#define stderr stderr
#endif

#ifdef stdout
#pragma push_macro ("stdout")
#define stdout stdout
#endif
#include "./File.h"
#include <assert.h>

namespace Core {

template <typename... Args>
constexpr u32 dbgln(Args const&... args)
{
    return MUST(Core::File::stderr().writeln(forward<Args const&>(args)...));
}

template <typename... Args>
constexpr u32 dbgwrite(Args const&... args) requires(sizeof...(Args) > 0)
{
    return MUST(Core::File::stderr().write(forward<Args const&>(args)...));
}

template <typename... Args>
constexpr u32 dprintln(StringView format, Args const&... args)
{
    auto parts = MUST(format.split_on("{}"));
    assert(sizeof...(Args) == parts.size() - 1);

    u32 size = 0;
    u32 i = 0;
    u32 results[] = {
        (size += dbgwrite(parts[i++], args))...,
        (size += dbgln(parts.last())),
    };
    (void)results;
    return size;
}

template <typename... Args>
constexpr u32 dprint(StringView format, Args const&... args)
{
    auto parts = MUST(format.split_on("{}"));
    assert(sizeof...(Args) == parts.size() - 1);

    u32 size = 0;
    u32 i = 0;
    u32 results[] = {
        (size += dbgwrite(parts[i++], args))...,
        (size += dbgwrite(parts.last())),
    };
    (void)results;
    return size;
}

}

using Core::dbgln;
using Core::dbgwrite;
using Core::dprintln;
using Core::dprint;

#ifdef stdout
#pragma pop_macro("stdout")
#endif

#ifdef stderr
#pragma pop_macro("stderr")
#endif

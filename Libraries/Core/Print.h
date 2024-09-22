#pragma once
#ifdef stderr
#pragma push_macro("stderr")
#define stderr stderr
#endif

#ifdef stdout
#pragma push_macro ("stdout")
#define stdout stdout
#endif
#include <Core/File.h>

namespace Core {

template <typename... Args>
constexpr u32 writeln(Args const&... args)
{
    return MUST(Core::File::stdout().writeln(forward<Args const&>(args)...));
}

template <typename... Args>
constexpr u32 dbgln(Args const&... args)
{
    return MUST(Core::File::stderr().writeln(forward<Args const&>(args)...));
}

template <typename... Args>
constexpr u32 outwrite(Args const&... args) requires(sizeof...(Args) > 0)
{
    return MUST(Core::File::stdout().write(forward<Args const&>(args)...));
}

template <typename... Args>
constexpr u32 dbgwrite(Args const&... args) requires(sizeof...(Args) > 0)
{
    return MUST(Core::File::stderr().write(forward<Args const&>(args)...));
}

}

using Core::dbgln;
using Core::dbgwrite;
using Core::outwrite;
using Core::writeln;

#ifdef stdout
#pragma pop_macro("stdout")
#endif

#ifdef stderr
#pragma pop_macro("stderr")
#endif

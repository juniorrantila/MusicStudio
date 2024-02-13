#pragma once
#include <Core/File.h>

namespace Core {

template <typename... Args>
constexpr u32 writeln(Args... args)
{
    return MUST(Core::File::stdout().writeln(args...));
}

template <typename... Args>
constexpr u32 dbgln(Args... args)
{
    return MUST(Core::File::stderr().writeln(args...));
}

template <typename... Args>
constexpr u32 outwrite(Args... args) requires(sizeof...(Args) > 0)
{
    return MUST(Core::File::stdout().write(args...));
}

template <typename... Args>
constexpr u32 dbgwrite(Args... args) requires(sizeof...(Args) > 0)
{
    return MUST(Core::File::stderr().write(args...));
}

}

using Core::dbgln;
using Core::dbgwrite;
using Core::outwrite;
using Core::writeln;

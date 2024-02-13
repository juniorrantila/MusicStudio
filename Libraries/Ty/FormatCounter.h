#pragma once
#include "Forward.h"

#include "ErrorOr.h"

namespace Ty {

struct FormatCounter {

    template <typename T>
    static ErrorOr<u32> count(T value)
    {
        auto dummy = FormatCounter();
        return TRY(Formatter<T>::write(dummy, value));
    }

    template <typename... Args>
    static ErrorOr<u32> count(Args... args) requires(
        sizeof...(args) > 1)
    {
        constexpr auto args_size = sizeof...(Args);
        ErrorOr<u32> parts[args_size] {
            count(args)...,
        };
        u32 total = 0;
        for (u32 i = 0; i < args_size; i++)
            total += TRY(parts[i]);
        return total;
    }

    static constexpr ErrorOr<u32> write(StringView view)
    {
        return view.size;
    }

    template <typename... Args>
    static constexpr ErrorOr<u32> write(Args... args) requires(
        sizeof...(args) > 1)
    {
        auto dummy = FormatCounter();
        constexpr auto const args_size = sizeof...(Args);
        ErrorOr<u32> parts[] {
            Formatter<Args>::write(dummy, args)...,
        };
        u32 sum = 0;
        for (u32 i = 0; i < args_size; i++)
            sum += TRY(parts[i]);
        return sum;
    }

    template <typename... Args>
    static constexpr ErrorOr<u32> writeln(Args... args)
    {
        return write(args..., "\n"sv);
    }
};

}

using Ty::FormatCounter;

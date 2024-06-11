#pragma once
#include "./Bytes.h"
#include "./Concepts.h"
#include "./Error.h"
#include "./ErrorOr.h"
#include "./Forward.h"
#include "./StringView.h"
#include "./Try.h"

namespace Ty {

namespace CompileError {
template <typename T>
consteval ErrorOr<u32> formatter_not_defined_for();
}

template <typename T>
requires(!is_trivially_copyable<T>) struct Formatter<T> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U&, T const&)
    {
        return CompileError::formatter_not_defined_for<T>();
    }
};

template <typename T>
requires is_trivially_copyable<T>
struct Formatter<T> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U&, T)
    {
        return CompileError::formatter_not_defined_for<T>();
    }
};

template <>
struct Formatter<StringView> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, StringView view)
    {
        return TRY(to.write(view));
    }
};

template <>
struct Formatter<u128> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u128 number)
    {
        if (number == 0) {
            return TRY(to.write("0"sv));
        }

        constexpr auto max_digits_in_u128 = 39;
        char buffer[max_digits_in_u128];
        u32 buffer_start = max_digits_in_u128;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = digit_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_digits_in_u128 - buffer_start;

        auto view = StringView(&buffer[buffer_start], buffer_size);
        return TRY(to.write(view));
    }

private:
    static constexpr char digit_to_character(u8 number)
    {
        return (char)('0' + number);
    }
};

template <>
struct Formatter<u64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u64 number)
    {
        if (number == 0) {
            return TRY(to.write("0"sv));
        }

        constexpr auto max_digits_in_u64 = 20;
        char buffer[max_digits_in_u64];
        u32 buffer_start = max_digits_in_u64;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = digit_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_digits_in_u64 - buffer_start;

        auto view = StringView(&buffer[buffer_start], buffer_size);
        return TRY(to.write(view));
    }

private:
    static constexpr char digit_to_character(u8 number)
    {
        return (char)('0' + number);
    }
};

template <>
struct Formatter<u32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u32 number)
    {
        return TRY(Formatter<u64>::write(to, number));
    }
};

template <>
struct Formatter<u16> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u16 number)
    {
        return TRY(Formatter<u64>::write(to, number));
    }
};

template <>
struct Formatter<i128> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i128 number)
    {
        u32 size = 0;
        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }

        size += TRY(Formatter<u128>::write(to, number));

        return size;
    }
};

template <>
struct Formatter<i64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i64 number)
    {
        u32 size = 0;
        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }

        size += TRY(Formatter<u64>::write(to, number));

        return size;
    }
};

template <>
struct Formatter<i32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i32 number)
    {
        return TRY(Formatter<i64>::write(to, number));
    }
};

template <>
struct Formatter<i16> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i16 number)
    {
        return TRY(Formatter<i64>::write(to, number));
    }
};

template <>
struct Formatter<f64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, f64 number)
    {
        u32 size = 0;

        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }
        auto integer_part = (u128)number;
        size += TRY(Formatter<u128>::write(to, integer_part));
        size += TRY(to.write("."sv));
        auto fraction_part = (number - (f64)integer_part);
        auto fraction = (u128)(fraction_part * 1000000000000000.0);
        if (fraction == 0) {
            size += TRY(to.write("0"sv));
            return size;
        }
        while (fraction % 10 == 0)
            fraction /= 10;
        size += TRY(Formatter<u128>::write(to, fraction));

        return size;
    }
};

template <>
struct Formatter<f32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, f32 number)
    {
        return TRY(Formatter<f64>::write(to, number));
    }
};

template <>
struct Formatter<char> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, char character)
    {
        return TRY(to.write(StringView(&character, 1)));
    }
};

template <>
struct Formatter<bool> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, bool value)
    {
        if (value)
            return TRY(to.write("true"sv));
        return TRY(to.write("false"sv));
    }
};

template <>
struct Formatter<Error> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Error error)
    {
        u32 size = 0;
        if (!error.function().is_empty())
            size += TRY(to.write(error.function(), ": "sv));
        if (auto message = error.user_message(); message.has_value()) {
            size += TRY(to.write(*message));
        }
        if (auto message = error.errno_message(); message.has_value()) {
            size += TRY(to.write(": "sv, *message));
        }
        if (!error.file().is_empty()) {
            size += TRY(to.write(" ["sv, error.file(), ":"sv,
                error.line_in_file(), "]"sv));
        }

        return size;
    }
};

template <typename T, typename E>
struct Formatter<ErrorOr<T, E>> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        ErrorOr<T, E> const& erroror)
    {
        auto size = TRY(to.write("ErrorOr("sv));

        if (erroror.is_error()) {
            size += TRY(to.write(erroror.error()));
        } else if (erroror.has_value()) {
            size += TRY(to.write(erroror.value()));
        } else {
            size += TRY(to.write("Moved"sv));
        }

        size += TRY(to.write(")"sv));
        return size;
    }
};

template <typename E>
struct Formatter<ErrorOr<StringView, E>> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        ErrorOr<StringView, E> const& erroror)
    {
        auto size = TRY(to.write("ErrorOr("sv));

        if (erroror.is_error()) {
            size += TRY(to.write(erroror.error()));
        } else if (erroror.has_value()) {
            size += TRY(to.write("\""sv, erroror.value(), "\""sv));
        } else {
            size += TRY(to.write("Moved"sv));
        }

        size += TRY(to.write(")"sv));
        return size;
    }
};

template <typename T>
struct Formatter<Optional<T>> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        Optional<T> const& maybe_value)
    {
        auto size = TRY(to.write("Optional("sv));

        if (maybe_value.has_value()) {
            size += TRY(to.write(maybe_value.value()));
        } else {
            size += TRY(to.write("None"sv));
        }

        size += TRY(to.write(")"sv));
        return size;
    }
};

template <>
struct Formatter<Optional<StringView>> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        Optional<StringView> const& maybe_value)
    {
        auto size = TRY(to.write("Optional("sv));

        if (maybe_value.has_value()) {
            size += TRY(
                to.write("\""sv, maybe_value.value(), "\""sv));
        } else {
            size += TRY(to.write("None"sv));
        }

        size += TRY(to.write(")"sv));
        return size;
    }
};

template <>
struct Ty::Formatter<Bytes> {
    template <typename U>
    requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Bytes value)
    {
        u32 size = 0;
        for (u32 i = 0; i < value.size(); i++) {
            u8 e = value[i];
            u8 upper = (e & 0xF0) >> 4;
            u8 lower = (e & 0x0F) >> 0;

            upper = upper > 9 ? upper + 'A' - 10 : upper + '0';
            lower = lower > 9 ? lower + 'A' - 10 : lower + '0';

            char nibbles[2] = {
                (char)upper,
                (char)lower,
            };
            size += TRY(to.write(StringView::from_parts(nibbles, 2)));
        }
        return size;
    }
};

}

using namespace Ty;

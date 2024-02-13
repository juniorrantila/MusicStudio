#include "Parse.h"
#include "Limits.h"
#include "Optional.h"
#include "StringView.h"

namespace Ty {

namespace {

Optional<u8> character_to_number(char character);

}

template <>
Optional<u16> Parse<u16>::from(StringView from)
{
    u32 result = 0;
    for (u32 i = 0; i < from.size; i++) {
        result *= 10;
        auto maybe_number = character_to_number(from[i]);
        if (!maybe_number.has_value())
            return {};
        result += maybe_number.value();
        if (result > Limits<u16>::max())
            return {};
    }
    return result;
}

template <>
Optional<u32> Parse<u32>::from(StringView from)
{
    u64 result = 0;
    for (u32 i = 0; i < from.size; i++) {
        result *= 10;
        auto maybe_number = character_to_number(from[i]);
        if (!maybe_number.has_value())
            return {};
        result += maybe_number.value();
        if (result > Limits<u32>::max())
            return {};
    }
    return result;
}

template <>
Optional<u64> Parse<u64>::from(StringView from)
{
    u128 result = 0;
    for (u32 i = 0; i < from.size; i++) {
        result *= 10;
        auto maybe_number = character_to_number(from[i]);
        if (!maybe_number.has_value())
            return {};
        result += maybe_number.value();
        if (result > Limits<u64>::max())
            return {};
    }
    return result;
}

template <>
Optional<f64> Parse<f64>::from(StringView from)
{
    if (!from.contains('.')) {
        auto value = Parse<u64>::from(from);
        if (value.has_value()) {
            return (f64)value.release_value();
        }
        return {};
    }

    bool add_to_fraction = false;
    u128 whole_part = 0;
    u128 fraction_part = 0;
    for (u32 i = 0; i < from.size; i++) {
        if (add_to_fraction) {
            fraction_part *= 10;
            auto maybe_number = character_to_number(from[i]);
            if (!maybe_number.has_value())
                return {};
            fraction_part += maybe_number.value();
            continue;
        }
        if (from[i] == '.') {
            add_to_fraction = true;
            continue;
        }

        whole_part *= 10;
        auto maybe_number = character_to_number(from[i]);
        if (!maybe_number.has_value())
            return {};
        whole_part += maybe_number.value();
        if (whole_part > Limits<u64>::max())
            return {};
    }

    auto digits_in = [](u128 value) {
        u128 digits = 0;

        do {
            digits++;
            value /= 10;
        } while (value != 0);

        return digits;
    };
    auto digits_in_fraction = digits_in(fraction_part);

    auto pow = [](u128 a, u128 b) {
        auto orig_a = a;
        while (--b != 0)
            a *= orig_a;
        return a;
    };
    auto fraction_divisor = pow(10, digits_in_fraction);

    return f64(
        whole_part + (f64(fraction_part) / f64(fraction_divisor)));
}

template <>
Optional<f32> Parse<f32>::from(StringView from)
{
    auto number = Parse<f64>::from(from);
    if (number.has_value())
        return (f32)number.release_value();
    return {};
}

namespace {

Optional<u8> character_to_number(char character)
{
    switch (character) {
    case '0' ... '9': return character - '0';
    default: return {};
    }
}

}

}

#pragma once
#include "./Base.h"
#include "./StringView.h"
#include "./TypeId.h"
#include <errno.h>

#include <Basic/Error.h>

namespace Ty {

struct Error {
    constexpr Error(KError e)
        : Error(kerror_strerror(e), kerror_function(e), kerror_file(e), kerror_line(e), string_domain, 0)
    {

    }

    static constexpr Error unimplemented(
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u16 line = __builtin_LINE())
    {
        return from_string_literal("unimplemented", function, file, line);
    }

    static Error from_leaky_string(StringView message,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line = __builtin_LINE());

    static constexpr Error from_string_literal(c_string message,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line = __builtin_LINE())
    {
        return { message, function, file, line, string_domain, 0 };
    }

    static Error from_string_literal_with_errno(c_string message, int code = errno,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line = __builtin_LINE())
    {
        return { message, function, file, line, code };
    }

    static Error from_errno(int code = errno,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line = __builtin_LINE())
    {
        return { nullptr, function, file, line, code };
    }

    template <typename T>
        requires __is_enum(T) && (sizeof(T) <= sizeof(u16))
    static Error from_enum(T value, c_string function = __builtin_FUNCTION(), c_string file = __builtin_FILE(), u32 line = __builtin_LINE())
    {
        c_string message = to_c_string(value);
        return { message, function, file, line, type_id<T>(), (u16)value };
    }

    constexpr StringView message() const
    {
        return user_message().or_else([this] {
            return errno_message().unwrap();
        });
    }

    constexpr Optional<StringView> user_message() const
    {
        if (!m_user_message)
            return {};
        return StringView::from_c_string(m_user_message);
    }

    constexpr static TypeId errno_domain = TypeId(0);
    constexpr static TypeId string_domain = TypeId(0xFFFF);

    constexpr TypeId domain() const
    {
        return m_domain;
    }

    template <typename T>
    constexpr T error_code() const
    {
        VERIFY(m_domain == type_id<T>());
        return (T)m_error_code;
    }

    constexpr Optional<int> errno_code() const
    {
        if (m_domain == errno_domain) {
            return (int)m_error_code;
        }
        return {};
    }

    Optional<StringView> errno_message() const;

    constexpr StringView function() const
    {
        return StringView::from_c_string(m_function);
    }

    constexpr StringView file() const
    {
        return StringView::from_c_string(m_file);
    }

    constexpr u32 line_in_file() const
    {
        return m_line;
    }

private:
    constexpr Error(c_string message, c_string function, c_string file, u32 line, TypeId domain, u16 code)
        : m_user_message(message)
        , m_function(function)
        , m_file(file)
        , m_line(line)
        , m_domain(domain)
        , m_error_code(code)
    {
    }

    constexpr Error(c_string message, c_string function, c_string file, u32 line, int errno_code)
        : Error(message, function, file, line, errno_domain, (u16)(errno_code < 0 ? -errno_code : errno_code))
    {
        VERIFY(__builtin_abs(errno_code) < 65536);
    }

    c_string m_user_message { 0 };
    c_string m_function { 0 };
    c_string m_file { 0 };
    u32 m_line { 0 };
    TypeId m_domain;
    u16 m_error_code { 0 };
};

}

using Ty::Error;

#pragma once
#include "./Base.h"
#include "./StringView.h"
#include <errno.h>

namespace Ty {

struct Error {
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
        return { message, function, file, line, 0 };
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

    constexpr Optional<int> errno_code() const
    {
        if (!m_errno)
            return {};
        return m_errno;
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
    constexpr Error(c_string message, c_string function, c_string file, u32 line, int errno_code)
        : m_user_message(message)
        , m_function(function)
        , m_file(file)
        , m_line(line)
        , m_errno(errno_code)
    {
    }

    c_string m_user_message { 0 };
    c_string m_function { 0 };
    c_string m_file { 0 };
    u32 m_line { 0 };
    int m_errno { 0 };
};

}

using Ty::Error;

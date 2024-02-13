#pragma once
#include <Ty/Base.h>
#include <Ty/Move.h>
#include <Ty/SmallCapture.h>
#include <Ty/SmallMap.h>
#include <Ty/SmallVector.h>
#include <Ty/StringBuffer.h>
#include <Ty/StringView.h>
#include <Ty/Try.h>

namespace CLI {

struct ArgumentParserError {
    ErrorOr<void> show() const;

    constexpr ArgumentParserError(StringBuffer&& buffer)
        : m_buffer(move(buffer))
        , m_state(State::Buffer)
    {
    }

    constexpr ArgumentParserError(Error error)
        : m_error(error)
        , m_state(State::Error)
    {
    }

    constexpr ArgumentParserError(ArgumentParserError&& other)
        : m_state(other.m_state)
    {
        switch (other.m_state) {
        case State::Buffer: {
            new (&m_buffer) StringBuffer(move(other.m_buffer));
        } break;
        case State::Error: {
            m_error = other.m_error;
        } break;
        case State::Invalid: break;
        }
        other.m_state = State::Invalid;
    }

    ~ArgumentParserError()
    {
        switch (m_state) {
        case State::Buffer: m_buffer.~StringBuffer(); break;
        case State::Error: m_error.~Error(); break;
        case State::Invalid: break;
        }
        m_state = State::Invalid;
    }

    constexpr StringView message() const
    {
        switch (m_state) {
        case State::Buffer: return m_buffer.view(); break;
        case State::Error: return m_error.message(); break;
        case State::Invalid: return "should never get here"sv;
        }
    }

private:
    union {
        StringBuffer m_buffer;
        Error m_error;
    };
    enum class State : u8 {
        Buffer,
        Error,
        Invalid,
    };
    State m_state;
};
using ArgumentParserResult = ErrorOr<void, ArgumentParserError>;

struct ArgumentParser {
    ErrorOr<void> add_flag(StringView long_name,
        StringView short_name, StringView explanation,
        SmallCapture<void()>&& callback)
    {
        auto id = flag_callbacks.size();
        TRY(flags.append({ long_name, short_name, explanation }));
        TRY(flag_callbacks.append(move(callback)));
        TRY(long_flag_ids.append(long_name, id));
        TRY(short_flag_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_option(StringView long_name,
        StringView short_name, StringView placeholder,
        StringView explanation,
        SmallCapture<void(c_string)>&& callback)
    {
        auto id = option_callbacks.size();
        TRY(options.append({
            long_name,
            short_name,
            explanation,
            placeholder,
        }));
        TRY(option_callbacks.append(move(callback)));

        TRY(long_option_ids.append(long_name, id));
        TRY(short_option_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_positional_argument(StringView placeholder,
        SmallCapture<void(c_string)>&& callback)
    {
        TRY(positional_placeholders.append(placeholder));
        TRY(positional_callbacks.append(move(callback)));

        return {};
    }

    ArgumentParserResult run(int argc, c_string argv[]) const;

    void print_usage_and_exit(c_string program_name,
        int exit_code = 0) const;

private:
    struct Flag {
        StringView long_name;
        StringView short_name;
        StringView explanation;
    };
    SmallVector<Flag> flags {};

    struct Option {
        StringView long_name;
        StringView short_name;
        StringView explanation;
        StringView placeholder;
    };
    SmallVector<Option> options {};

    SmallMap<StringView, u32> short_flag_ids {};
    SmallMap<StringView, u32> long_flag_ids {};

    SmallMap<StringView, u32> short_option_ids {};
    SmallMap<StringView, u32> long_option_ids {};

    SmallVector<SmallCapture<void()>> flag_callbacks {};
    SmallVector<SmallCapture<void(c_string)>> option_callbacks {};

    SmallVector<StringView> positional_placeholders {};
    SmallVector<SmallCapture<void(c_string)>>
        positional_callbacks {};
};
}

#pragma once
#include <LibTy/ErrorOr.h>
#include <LibTy/StringBuffer.h>

namespace CLI {

struct ArgumentParserError {
    ErrorOr<void> show() const;

    ArgumentParserError(StringBuffer&& buffer);
    ArgumentParserError(Error error);

    ArgumentParserError(ArgumentParserError&& other);

    ~ArgumentParserError();

    StringView message() const;

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

}

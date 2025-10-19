#include "./ArgumentParser.h"

#include <LibCore/File.h>

namespace CLI {

ArgumentParserError::ArgumentParserError(StringBuffer&& buffer)
    : m_buffer(move(buffer))
    , m_state(State::Buffer)
{
}

ArgumentParserError::ArgumentParserError(Error error)
    : m_error(error)
    , m_state(State::Error)
{
}

ArgumentParserError::ArgumentParserError(ArgumentParserError&& other)
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

ArgumentParserError::~ArgumentParserError()
{
    switch (m_state) {
    case State::Buffer: m_buffer.~StringBuffer(); break;
    case State::Error: m_error.~Error(); break;
    case State::Invalid: break;
    }
    m_state = State::Invalid;
}

StringView ArgumentParserError::message() const
{
    switch (m_state) {
    case State::Buffer: return m_buffer.view(); break;
    case State::Error: return m_error.message(); break;
    case State::Invalid: return "should never get here"sv;
    }
}

ErrorOr<void> ArgumentParserError::show() const
{
    switch (m_state) {
    case State::Buffer: {
        TRY(Core::File::stderr().write(m_buffer));
    } break;
    case State::Error: {
        TRY(Core::File::stderr().write(m_error));
    } break;
    case State::Invalid: break;
    }

    return {};
}

}

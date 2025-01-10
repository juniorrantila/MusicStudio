#include "./Error.h"
#include "./StringBuffer.h"
#include <string.h>

namespace Ty {

Error Error::from_leaky_string(StringView message, c_string function, c_string file, u32 line)
{
    auto buf = MUST(StringBuffer::create_saturated_fill(message, "\0"sv));
    return Error::from_string_literal(buf.leak(), function, file, line);
}


Optional<StringView> Error::errno_message() const
{
    auto e = errno_code();
    if (!e.has_value()) {
        return {};
    }
    auto code = strerror(e.value());
    if (!code)
        return {};
    return StringView::from_c_string(code);
}

}

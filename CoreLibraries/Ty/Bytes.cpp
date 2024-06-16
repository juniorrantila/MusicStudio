#include "./Bytes.h"

#include "./StringView.h"
#include "./StringBuffer.h"

namespace Ty {

StringView Bytes::as_view() const
{
    return StringView::from_parts((char const*)m_data, m_size);
}

ErrorOr<StringBuffer> Bytes::as_c_source_file(StringView variable_name) const
{
    auto buffer = TRY(StringBuffer::create());

    TRY(buffer.writeln("inline static const __SIZE_TYPE__ "sv, variable_name, "_size = "sv, m_size, "ULL;"sv));
    TRY(buffer.writeln("extern unsigned char "sv, variable_name, "["sv, m_size, "ULL];"sv));
    TRY(buffer.writeln());

    auto macro_name = TRY(StringBuffer::create_fill(variable_name));
    TRY(buffer.writeln("#ifdef "sv, macro_name.uppercase(), "_IMPLEMENTATION"sv));

    TRY(buffer.writeln("unsigned char "sv, variable_name, "["sv, m_size, "ULL] = {"sv));
    usize bytes_written = 0;
    for (usize i = 0; i < m_size; i++) {
        if (bytes_written == 0) {
            TRY(buffer.write("    "sv));
        }
        TRY(buffer.write((unsigned)m_data[i]));
        TRY(buffer.write(", "sv));
        bytes_written += 1;
        if (bytes_written == 16) {
            TRY(buffer.write("\n"sv));
            bytes_written = 0;
        }
    }
    if (bytes_written != 0) {
        TRY(buffer.writeln());
    }
    TRY(buffer.writeln("};"sv));

    TRY(buffer.writeln("#endif"sv));

    return buffer;
}

}

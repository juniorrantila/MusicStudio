#pragma once
#include "./Forward.h"

#include "./Base.h"
#include "./Verify.h"

namespace Ty {

struct Bytes {
    constexpr Bytes() = default;

    constexpr Bytes(void const* data, usize size)
        : m_data((u8 const*)data)
        , m_size(size)
    {
    }

    constexpr u8 const* begin() const { return m_data; }
    constexpr u8 const* end() const { return &m_data[m_size]; }

    constexpr u8 const& operator[](usize index) const
    {
        VERIFY(index < m_size);
        return m_data[index];
    }

    constexpr u8 const* data() const { return m_data; }
    constexpr usize size() const { return m_size; }
    StringView as_view() const;

    ErrorOr<StringBuffer> as_c_source_file(StringView variable_name) const;

private:
    u8 const* m_data { nullptr };
    usize m_size { 0 };
};

}

#pragma once
#include "Base.h"
#include "Verify.h"

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

    u8 const* data() const { return m_data; }
    usize size() const { return m_size; }

private:
    u8 const* m_data { nullptr };
    usize m_size { 0 };
};

}

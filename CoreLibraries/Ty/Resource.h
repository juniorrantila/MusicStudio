#pragma once
#include "./Bytes.h"
#include "./StringView.h"
#include "./Verify.h"

namespace Ty {

struct Resource {
    static Resource create_with_resolved_path(StringView path, Bytes bytes)
    {
        return Resource(path, bytes);
    }

    StringView resolved_path() const { return m_resolved_path; }
    Bytes bytes() const { return m_bytes; }
    StringView view() const { return StringView::from_parts((char const*)m_bytes.data(), m_bytes.size()); }

    u8 const* data() const { return m_bytes.data(); }
    usize size() const { return m_bytes.size(); }

private:
    constexpr Resource(StringView resolved_path, Bytes bytes)
        : m_resolved_path(resolved_path)
        , m_bytes(bytes)
    {
    }

    StringView m_resolved_path {};
    Bytes m_bytes {};
};

}

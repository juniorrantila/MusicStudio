#pragma once
#include <Basic/Bytes.h>

#include <LibTy/StringView.h>
#include <LibTy/StringBuffer.h>

namespace FS {

struct ResourceView {
    static constexpr ResourceView create_with_resolved_path(StringView path, Bytes bytes)
    {
        return ResourceView(path, bytes);
    }

    constexpr StringView resolved_path() const { return m_resolved_path; }
    constexpr Bytes bytes() const { return m_bytes; }

    StringView view() const { return StringView::from_parts((char const*)m_bytes.items, m_bytes.count); }

    u8 const* data() const { return m_bytes.items; }
    usize size() const { return m_bytes.count; }

private:
    constexpr ResourceView(StringView resolved_path, Bytes bytes)
        : m_resolved_path(resolved_path)
        , m_bytes(bytes)
    {
    }

    StringView m_resolved_path {};
    Bytes m_bytes {};
};

struct Resource {
    static Resource create_with_resolved_path(StringBuffer path, StringBuffer bytes)
    {
        return Resource(move(path), move(bytes));
    }

    StringView resolved_path() const { return m_resolved_path.view(); }
    Bytes bytes() const { return m_bytes.view().as_bytes(); }
    StringView view() const { return StringView::from_parts((char const*)m_bytes.data(), m_bytes.size()); }

    u8 const* data() const { return (u8 const*)m_bytes.data(); }
    usize size() const { return m_bytes.size(); }

    ResourceView resource_view() const
    {
        return ResourceView::create_with_resolved_path(m_resolved_path.view(), bytes());
    }

private:
    constexpr Resource(StringBuffer resolved_path, StringBuffer bytes)
        : m_resolved_path(move(resolved_path))
        , m_bytes(move(bytes))
    {
    }

    StringBuffer m_resolved_path {};
    StringBuffer m_bytes {};
};

}

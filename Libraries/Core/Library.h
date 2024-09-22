#pragma once
#include <Ty/Base.h>
#include <Ty/ErrorOr.h>
#include <Ty/Try.h>

namespace Core {

struct Library {

    Library(Library&& other)
        : m_handle(other.m_handle)
    {
        other.invalidate();
    }
    ~Library()
    {
        if (!is_valid())
            return;
        destroy();
        invalidate();
    }

    Library& operator=(Library&& other)
    {
        if (this == &other)
            return *this;
        if (is_valid())
            this->~Library();
        m_handle = other.m_handle;
        other.invalidate();
        return *this;
    }
    
    static ErrorOr<Library> open_local(c_string path);
    constexpr bool is_valid() const { return m_handle != nullptr; }

    template <typename Signature>
    ErrorOr<Signature> fetch_symbol(c_string name) const
    {
        return (Signature)TRY(fetch_symbol(name));
    }

private:
    constexpr Library(void* handle)
        : m_handle(handle)
    {
    }

    void destroy();

    ErrorOr<void*> fetch_symbol(c_string) const;

    void invalidate() { m_handle = nullptr; }

    void* m_handle { nullptr };
};

}

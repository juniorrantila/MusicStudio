#pragma once
#include "./Base.h"
#include "./Traits.h"
#include "./Verify.h"
#include "./Formatter.h"
#include "./ReverseIterator.h"

namespace Ty {

template <typename T>
struct View {
    constexpr View() = default;

    constexpr View(T* data, usize size)
        : m_data(data)
        , m_size(size)
    {
    }

    T& operator[](usize index)
    {
        VERIFY(index < size());
        VERIFY(m_data);
        return m_data[index];
    }

    T const& operator[](usize index) const 
    {
        VERIFY(index < size());
        VERIFY(m_data);
        return m_data[index];
    }

    Optional<T> at(usize index) const
    {
        if (index >= size()) {
            return {};
        }
        return m_data[index];
    }

    constexpr bool is_empty() const { return size() == 0; }

    constexpr T* begin()
    {
        VERIFY(is_empty() || m_data);
        return m_data;
    }

    constexpr T* end()
    {
        VERIFY(is_empty() || m_data);
        return &m_data[m_size];
    }

    constexpr T const* begin() const
    {
        VERIFY(is_empty() || m_data);
        return m_data;
    }
    constexpr T const* end() const
    { 
        VERIFY(is_empty() || m_data);
        return &m_data[m_size];
    }

    constexpr ReverseIterator<T const> in_reverse() const
    {
        return { begin(), end() };
    }

    constexpr ReverseIterator<T> in_reverse() { return { begin(), end() }; }

    constexpr usize size() const { return m_size; }

    constexpr usize byte_size() const { return size() * sizeof(T); }

    constexpr T* data()
    {
        VERIFY(m_data);
        return m_data;
    }
    constexpr T const* data() const
    {
        VERIFY(m_data);
        return m_data;
    }

    constexpr T const& last() const
    {
        return *(end() - 1);
    }

    constexpr T& last()
    {
        return *(end() - 1);
    }

    constexpr View<T> part(u32 start, u32 end) const
    {
        return { &m_data[start], end - start };
    }

    constexpr View<T> sub_view(u32 start, u32 size) const
    {
        auto remaining = this->size() - start;
        if (remaining < size) [[unlikely]] {
            size = remaining;
        }
        return { &m_data[start], size };
    }

    constexpr View<const T> as_const() const { return View<const T>(m_data, m_size); }

    constexpr operator View<const T>() { return as_const(); }

    constexpr Bytes as_bytes() const { return Bytes(data(), size()); }

    View<T> shrink(usize size) const
    {
        return {
            m_data,
            m_size - size,
        };
    }

    View<T>& assign_from(View<T> other)
    {
        VERIFY(other.size() >= size());
        for (usize i = 0; i < size(); i++) {
            m_data[i] = other[i];
        }
        return *this;
    }

    View zero()
    {
        __builtin_memset(m_data, 0, sizeof(T) * m_size);
        return *this;
    }

private:
    T* m_data { 0 };
    usize m_size { 0 };
};

}

template <typename T>
struct Ty::Formatter<Ty::View<const T>> {
    template <typename U>
    requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Ty::View<T> view)
    {
        auto size = TRY(to.write("[ "sv));
        for (auto const& e : view)
            size += TRY(to.write(e, " "sv));
        size += TRY(to.write("]"sv));
        return size;
    }
};


template <typename T>
struct Ty::Formatter<Ty::View<T>> {
    template <typename U>
    requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Ty::View<T> view)
    {
        return TRY(to.write(view.as_const()));
    }
};

template <>
struct Ty::Formatter<Ty::View<const u8>> {
    template <typename U>
    requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Ty::View<const u8> view)
    {
        auto size = TRY(to.write("[ "sv));
        for (u8 e : view) {
            size += TRY(to.write("0x"sv));
            u8 upper = (e & 0xF0) >> 4;
            u8 lower = (e & 0x0F) >> 0;

            upper = upper > 9 ? upper + 'A' - 10 : upper + '0';
            lower = lower > 9 ? lower + 'A' - 10 : lower + '0';

            char bytes[] = {
                (char)upper,
                (char)lower,
                ' ',
            };
            size += TRY(to.write(StringView::from_parts(bytes, sizeof(bytes))));
        }
        size += TRY(to.write("]"sv));
        return size;
    }
};

using Ty::View;

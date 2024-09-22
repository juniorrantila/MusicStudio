#pragma once

namespace Ty {

template <typename T>
struct ReverseIterator {
    struct PointerLike {
        T* value;

        constexpr T& operator*() const { return *(value - 1); }

        constexpr T* operator++() { return value--; }

        constexpr bool operator==(PointerLike other) const
        {
            return value == other.value;
        }
    };
    T* first;
    T* last;

    PointerLike begin() const { return PointerLike { last }; }
    PointerLike end() const { return PointerLike { first }; }
};

}

using Ty::ReverseIterator;

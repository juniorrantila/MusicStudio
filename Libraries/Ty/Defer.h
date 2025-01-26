#pragma once

namespace Ty {

template <typename F>
class Defer {
public:
    constexpr Defer(F callback)
        : callback(callback)
    {
    }

    constexpr void run()
    {
        callback();
        disarm();
    }

    constexpr void disarm()
    {
        m_is_armed = false;
    }

    constexpr ~Defer() {
        if (m_is_armed) {
            callback();
        }
    }

private:
    F callback;
    bool m_is_armed { true };
};

}

using Ty::Defer;

#define TY_DEFER_CAT2(a, b) a ## b
#define TY_DEFER_CAT(a, b) TY_DEFER_CAT2(a, b)
#define defer Defer TY_DEFER_CAT(defer_at_line_, __LINE__) =

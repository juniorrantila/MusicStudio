#pragma once

namespace Ty {

template <typename F>
class ScopeGuard {
public:
    constexpr ScopeGuard(F callback)
        : callback(callback)
    {
    }

    ~ScopeGuard()
    {
        if (!m_armed)
            return;
        callback();
    }

    void disarm() { m_armed = false; }

private:
    F callback;
    bool m_armed { true };
};

}

using namespace Ty;

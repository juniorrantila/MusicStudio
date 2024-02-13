#pragma once

namespace Ty {

template <typename F>
class Defer {
public:
    constexpr Defer(F callback)
        : callback(callback)
    {
    }

    ~Defer() { callback(); }

private:
    F callback;
};

}

using namespace Ty;

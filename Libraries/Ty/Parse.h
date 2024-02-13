#pragma once
#include "Forward.h"

#include "Base.h"

namespace Ty {

template <typename T>
struct Parse {
    static Optional<T> from(StringView);
};

}

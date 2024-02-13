#pragma once
#include "Hardware.h"

namespace Ty {

struct Threads {
    static u32 in_machine() { return Hardware::the().threads(); }
};

}

using Ty::Threads;

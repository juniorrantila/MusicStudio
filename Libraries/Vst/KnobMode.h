#pragma once
#include <JR/Types.h>

namespace Vst {

enum class KnobMode : u8 {
    Circular,
    CircularRelative,
    Linear,
};
constexpr char const* knob_mode_string(KnobMode mode)
{
    using enum KnobMode;
    switch (mode) {
    case Circular:
        return "Circular";
    case CircularRelative:
        return "CircularRelative";
    case Linear:
        return "Linear";
    }
}

}

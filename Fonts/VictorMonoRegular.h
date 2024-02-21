#pragma once
#include <Ty/Base.h>

namespace Fonts {

namespace Internal {
#include "./VictorMono-Regular.ttf.h"
}

static inline u8* victor_mono_regular_ttf = Internal::VictorMono_Regular_ttf;
static u32 victor_mono_regular_ttf_len = Internal::VictorMono_Regular_ttf_len;

}

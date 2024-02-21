#pragma once
#include <Ty/Base.h>

namespace Fonts {

namespace Internal {
#include "./OxaniumLight/Oxanium-Light.ttf.h"
}

static inline u8* oxanium_light_ttf = Internal::Oxanium_Light_ttf;
static u32 oxanium_light_ttf_len = Internal::Oxanium_Light_ttf_len;

}

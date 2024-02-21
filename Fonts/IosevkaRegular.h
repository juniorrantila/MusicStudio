#pragma once
#include <Ty/Base.h>

namespace Fonts {

namespace Internal {
#include "./iosevka-regular.ttf.h"
}

static inline u8* iosevka_regular_ttf = Internal::iosevka_regular_ttf;
static u32 iosevka_regular_ttf_len = Internal::iosevka_regular_ttf_len;

}

#pragma once
#include <LibCore/FSVolume.h>

struct Fonts {
    [[nodiscard]] static bool add_to_volume(FSVolume*);
};

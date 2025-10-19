#pragma once
#include <LibCore/Forward.h>

typedef enum {
    UseBakedShaders_No,
    UseBakedShaders_Yes,
} UseBakedShaders;

struct Shaders {
    static bool add_to_volume(FSVolume*, UseBakedShaders);
};

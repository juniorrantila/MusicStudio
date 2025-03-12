#pragma once
#include <FS/Forward.h>

typedef enum {
    UseBakedShaders_No,
    UseBakedShaders_Yes,
} UseBakedShaders;

struct Shaders {
    static void add_to_bundle(FS::Bundle&);
    static bool add_to_volume(FSVolume*, UseBakedShaders);
};

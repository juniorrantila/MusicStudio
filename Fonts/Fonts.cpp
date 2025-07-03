#include "./Fonts.h"

#include <FS/FSVolume.h>

bool Fonts::add_to_volume(FSVolume* volume)
{
    static char const oxanium_light_bytes[] = {
        #embed "./OxaniumLight/Oxanium-Light.ttf"
    };
    if (!fs_volume_mount(volume,
        fs_virtual_open("Fonts/OxaniumLight/Oxanium-Light.ttf"s, string_slice(oxanium_light_bytes, sizeof(oxanium_light_bytes))),
        nullptr
    )) return false;

    static char const victor_mono_bytes[] = {
        #embed "./VictorMono-Regular.ttf"
    };
    if (!fs_volume_mount(volume,
        fs_virtual_open("Fonts/VictorMono-Regular.ttf"s, string_slice(victor_mono_bytes, sizeof(victor_mono_bytes))),
        nullptr
    )) return false;

    static char const iosevka_bytes[] = {
        #embed "./iosevka-regular.ttf"
    };
    if (!fs_volume_mount(volume,
        fs_virtual_open("Fonts/iosevka-regular.ttf"s, string_slice(iosevka_bytes, sizeof(iosevka_bytes))),
        nullptr
    )) return false;

    return true;
}

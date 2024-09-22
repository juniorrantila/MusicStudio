#include "./Fonts.h"

#include <FS/Resource.h>
#include <FS/Bundle.h>

static u8 const oxanium_light_bytes[] = {
#embed "./OxaniumLight/Oxanium-Light.ttf"
};
static constexpr auto oxanium_light_ttf = FS::ResourceView::create_with_resolved_path(
    "Fonts/OxaniumLight/Oxanium-Light.ttf"sv,
    Bytes(oxanium_light_bytes, sizeof(oxanium_light_bytes))
);

static u8 const victor_mono_bytes[] = {
#embed "./VictorMono-Regular.ttf"
};
static constexpr auto victor_mono_ttf = FS::ResourceView::create_with_resolved_path(
    "Fonts/VictorMono-Regular.ttf"sv,
    Bytes(victor_mono_bytes, sizeof(victor_mono_bytes))
);

static u8 const iosevka_bytes[] = {
#embed "./iosevka-regular.ttf"
};
static constexpr auto iosevka_ttf = FS::ResourceView::create_with_resolved_path(
    "Fonts/iosevka-regular.ttf"sv,
    Bytes(iosevka_bytes, sizeof(iosevka_bytes))
);

void Fonts::add_to_bundle(FS::Bundle& bundle)
{
    bundle.unsafe_add_resource(oxanium_light_ttf);
    bundle.unsafe_add_resource(victor_mono_ttf);
    bundle.unsafe_add_resource(iosevka_ttf);
}

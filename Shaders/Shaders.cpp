#include "./Shaders.h"

#include <Basic/PageAllocator.h>
#include <LibCore/FSVolume.h>
#include <LibCore/Resource.h>

#define BUNDLE(path, bytes)                         \
    FS::ResourceView::create_with_resolved_path(    \
        path,                                       \
        Bytes(bytes, sizeof(bytes))                 \
    )

static u8 const simple_vert_bytes[] = {
#embed "./simple.vert"
};
static constexpr auto simple_vert = BUNDLE(
    "Shaders/simple.vert"sv,
    simple_vert_bytes
);

static u8 const simple_color_frag_bytes[] = {
#embed "./color.frag"
};
static constexpr auto simple_color_frag = BUNDLE(
    "Shaders/color.frag"sv,
    simple_color_frag_bytes
);

static u8 const simple_frag_bytes[] = {
#embed "./simple.frag"
};
static constexpr auto simple_frag = BUNDLE(
    "Shaders/simple.frag"sv,
    simple_frag_bytes
);

#undef BUNDLE

static bool add(FS::ResourceView view, FSVolume*, UseBakedShaders);

bool Shaders::add_to_volume(FSVolume* volume, UseBakedShaders use_baked_shaders)
{
    if (!add(simple_vert, volume, use_baked_shaders))
        return false;
    if (!add(simple_frag, volume, use_baked_shaders))
        return false;
    if (!add(simple_color_frag, volume, use_baked_shaders))
        return false;
    return true;
}

static bool add(FS::ResourceView view, FSVolume* volume, UseBakedShaders use_baked_shaders)
{
    FSFile file {};

    switch (use_baked_shaders) {
    case UseBakedShaders_Yes:
        file = fs_virtual_open(view.resolved_path(), view.view());
        break;
    case UseBakedShaders_No:
        if (!fs_system_open(page_allocator(), view.resolved_path(), &file))
            return false;
        break;
    }

    if (!fs_volume_mount(volume, file, nullptr))
        return false;
    return true;
}

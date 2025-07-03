#include "./Shaders.h"

#include <FS/FSVolume.h>
#include <FS/Resource.h>
#include <Ty2/PageAllocator.h>

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

#undef BUNDLE

bool Shaders::add_to_volume(FSVolume* volume, UseBakedShaders use_baked_shaders)
{
    FSFile vert {};
    FSFile frag {};

    switch (use_baked_shaders) {
    case UseBakedShaders_Yes:
        vert = fs_virtual_open(simple_vert.resolved_path(), simple_vert.view());
        frag = fs_virtual_open(simple_color_frag.resolved_path(), simple_color_frag.view());
        break;
    case UseBakedShaders_No:
        if (!fs_system_open(page_allocator(), simple_vert.resolved_path(), &vert))
            return false;
        if (!fs_system_open(page_allocator(), simple_color_frag.resolved_path(), &frag))
            return false;
        break;
    }

    if (!fs_volume_mount(volume, vert, nullptr))
        return false;

    if (!fs_volume_mount(volume, frag, nullptr))
        return false;

    return true;
}

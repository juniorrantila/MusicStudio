#include "./Shaders.h"

#include <FS/Resource.h>
#include <FS/Bundle.h>

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

static u8 const simple_image_frag_bytes[] = {
#embed "./image.frag"
};
static constexpr auto simple_image_frag = BUNDLE(
    "Shaders/image.frag"sv,
    simple_image_frag_bytes
);

static u8 const simple_text_frag_bytes[] = {
#embed "./text.frag"
};
static constexpr auto simple_text_frag = BUNDLE(
    "Shaders/text.frag"sv,
    simple_text_frag_bytes
);

static u8 const simple_epic_frag_bytes[] = {
#embed "./epic.frag"
};
static constexpr auto simple_epic_frag = BUNDLE(
    "Shaders/epic.frag"sv,
    simple_epic_frag_bytes
);

#undef BUNDLE

void Shaders::add_to_bundle(FS::Bundle& bundle)
{
    bundle.unsafe_add_resource(simple_vert);
    bundle.unsafe_add_resource(simple_color_frag);
    bundle.unsafe_add_resource(simple_image_frag);
    bundle.unsafe_add_resource(simple_text_frag);
    bundle.unsafe_add_resource(simple_epic_frag);
}

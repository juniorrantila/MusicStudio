#include "./UI.h"

#include "../State.h"

#include <Basic/Bits.h>
#include <Basic/Context.h>
#include <Basic/Defer.h>

#include <LibCore/Actor.h>
#include <LibLayout2/UI.h>
#include <LibLayout2/Layout.h>

static consteval LayoutColor hex_to_color(u32 color)
{
    LayoutColor result;
    f32 r = (f32)((u32)(color>>(3*8))&0xFF);
    f32 g = (f32)((u32)(color>>(2*8))&0xFF);
    f32 b = (f32)((u32)(color>>(1*8))&0xFF);
    f32 a = (f32)((u32)(color>>(0*8))&0xFF);
    result.r = r/255.0f;
    result.g = g/255.0f;
    result.b = b/255.0f;
    result.a = a/255.0f;
    return result;
}

[[maybe_unused]] static constexpr LayoutColor cyan = { .r = 0.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor magenta = { .r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor yellow = { .r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor red = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor green = { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor blue = { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
[[maybe_unused]] static constexpr LayoutColor border_color = hex_to_color(0x2C3337FF);
[[maybe_unused]] static constexpr LayoutColor outline_color = hex_to_color(0x4B5255FF);
[[maybe_unused]] static constexpr LayoutColor background_color = hex_to_color(0x181F23FF);
[[maybe_unused]] static constexpr LayoutColor button_color = hex_to_color(0x99A89FFF);
[[maybe_unused]] static constexpr LayoutColor button_hover_color = hex_to_color(0x33A89FFF);
[[maybe_unused]] static constexpr LayoutColor gray_background = hex_to_color(0x646A71FF);
[[maybe_unused]] static constexpr LayoutColor gray = hex_to_color(0xAAAAAAAA);
[[maybe_unused]] static constexpr LayoutColor black = hex_to_color(0x000000FF);
[[maybe_unused]] static constexpr LayoutColor toolbar_color = hex_to_color(0x5B6265FF);

C_API void ui_actor_frame(StableLayout* stable, TransLayout* trans);
C_API void ui_actor_frame(StableLayout* stable, TransLayout* trans)
{
    VERIFY(ty_is_initialized(stable));
    ty_trans_migrate(trans);

    auto* l = &stable->layout;
    if (box_begin(l)) {
        defer [=]{ box_end(l); };
        box_color(l, background_color);
        box_layout_direction(l, LayoutDirection_Main);

        // Top bar
        if (box_begin(l)) {
            defer [=] { box_end(l); };
            box_height(l, 28);
            box_color(l, toolbar_color);
        }

        // Content
        if (box_begin(l)) {
            defer [=] { box_end(l); };
            box_layout_direction(l, LayoutDirection_Cross);
            box_child_gap(l, 2);

            // Sidebar
            if (box_begin(l)) {
                defer [=] { box_end(l); };
                box_color(l, gray_background);
                box_width(l, 200);
                box_layout_direction(l, LayoutDirection_Main);
                box_child_gap(l, 2);

                for (u32 i = 0; i < 8; i++) {
                    if (box_begin(l)) {
                        defer [=] { box_end(l); };
                        box_height(l, 32);
                        box_color(l, button_color);
                        if (box_hovered(l)) {
                            box_color(l, button_hover_color);
                        }
                        if (box_mouse_down(l)) {
                            box_color(l, toolbar_color);
                        }
                        if (box_pressed(l)) {
                            infof("box pressed");
                        }
                    }
                }
            }

            // Tracker
            if (box_begin(l)) {
                defer [=] { box_end(l); };
                box_color(l, background_color);
                box_layout_direction(l, LayoutDirection_Main);
                box_child_gap(l, 2);
            }
        }
    }
}

C_API bool ui_actor_init(Actor* actor, FSVolume* volume, bool use_auto_reload)
{
    if (!use_auto_reload) {
        *actor = actor_init((void(*)(void))ui_actor_frame);
        return true;
    }

    auto path = actor_library_path("music-studio-ui");
    FileID file;
    if (!fs_volume_find(volume, path, &file)) {
        errorf("could not find '%.*s'", (int)path.count, path.items);
        return false;
    }

    if (!actor_init_reloadable(actor, volume, file, "ui_actor_frame")) {
        errorf("could not create actor from library");
        return false;
    }
    return true;
}

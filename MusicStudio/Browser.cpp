#include "./Context.h"

#include "./Color.h"
#include "./UI.h"

#include <Layout/Sugar.h>
#include <Core/Print.h>

using namespace LayoutSugar;

void browser(Context* context)
{
    box({
        .id = "Browser"sv,
        .padding = {},
        .sizing = {
            .width = sizing_fixed(240),
            .height = sizing_grow(),
        },
        .color = {},
    }, [&]{
        vstack([&]{
            hstack({
                .id = {},
                .padding = {
                    .left = 16,
                    .right = 16,
                    .top = 16,
                    .bottom = 16,
                },
                .child_gap = 4,
                .child_alignment = {},
                .color = COLOR_BACKGROUND,
            }, [&]{
                center_button("Foo"sv, context->main_typeface, 0, [](Clay_ElementId, Clay_PointerData data, iptr){
                    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                        dprintln("Pressed Foo!");
                    }
                });
                center_button("Bar"sv, context->main_typeface, 0, [](Clay_ElementId, Clay_PointerData data, iptr){
                    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                        dprintln("Pressed Bar!");
                    }
                });
                center_button("Baz"sv, context->main_typeface, 0, [](Clay_ElementId, Clay_PointerData data, iptr){
                    if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                        dprintln("Pressed Baz!");
                    }
                });
            });
        });
    });
}

#include "./Context.h"

#include "./Color.h"

#include <LibLayout/Layout.h>
#include <LibLayout/Sugar.h>

using namespace LayoutSugar;

void pinboard(MSContext* context)
{
    Element().config([=]{
        id("PinBoard"sv);
        layout({
            .sizing = {
                .width = sizing_fit(),
                .height = sizing_fit(),
            },
            .padding = {
                .left = 0,
                .right = 0,
                .top = 0,
                .bottom = 0,
            },
            .childGap = 2,
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_CENTER,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
        rectangle({
            .color = COLOR_BACKGROUND,
            .cornerRadius = {},
        });
    }).body([=]{
        piano_roll_vertical(context);
        piano_tracker(context);
    });
}

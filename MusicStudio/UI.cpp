#include "./UI.h"

#include "./Color.h"

#include <Layout/Layout.h>
#include <Layout/Sugar.h>

using namespace LayoutSugar;

void button(ButtonConfig config)
{
    box(config.id, [&]{
        return BoxConfig{
            .corner_radius = { 2, 2, 2, 2 },
            .padding = {
                .left = 8,
                .right = 8,
                .top = 8,
                .bottom = 8,
            },
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_fit(),
            },
            .color = Clay_Hovered() ? COLOR_BLUE : COLOR_BUTTON,
            .child_alignment = config.text_align,
            .on_hover = config.on_hover,
            .on_hover_user = config.on_hover_user,
        };
    }, [=]{
        text(config.text, {
            .textColor = { 255, 255, 255, 255 },
            .fontId = config.font_id,
            .fontSize = 16,
            .letterSpacing = 0,
            .lineHeight = 0,
            .wrapMode = CLAY_TEXT_WRAP_NONE
        });
    });
}

void center_button(ID text, u16 font_id, iptr user, void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr))
{
    button({
        .id = text,
        .text = text.text,
        .font_id = font_id,
        .text_align = {
            .x = CLAY_ALIGN_X_CENTER,
            .y = CLAY_ALIGN_Y_CENTER,
        },
        .on_hover_user = user,
        .on_hover = on_hover,
    });
}

void left_button(ID text, u16 font_id, iptr user, void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr))
{
    button({
        .id = text,
        .text = text.text,
        .font_id = font_id,
        .text_align = {
            .x = CLAY_ALIGN_X_LEFT,
            .y = CLAY_ALIGN_Y_CENTER,
        },
        .on_hover_user = user,
        .on_hover = on_hover,
    });
}

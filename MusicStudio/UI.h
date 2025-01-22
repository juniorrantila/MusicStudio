#pragma once
#include <Layout/Layout.h>
#include <Layout/Sugar.h>

struct ButtonConfig {
    LayoutSugar::ID id;
    StringView text;
    u16 font_id;
    Clay_ChildAlignment text_align;
    iptr on_hover_user;
    void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr);
};

void button(ButtonConfig config);
void center_button(LayoutSugar::ID text, u16 font_id, iptr user, void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr));
void left_button(LayoutSugar::ID text, u16 font_id, iptr user, void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr));

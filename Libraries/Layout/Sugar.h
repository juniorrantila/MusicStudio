#pragma once
#include <Clay/Clay.h>

namespace LayoutSugar {

static Clay_RectangleElementConfig rectangle(Clay_RectangleElementConfig config)
{
    Clay__AttachElementConfig(Clay_ElementConfigUnion {
            .rectangleElementConfig = Clay__StoreRectangleElementConfig(config),
        },
        CLAY__ELEMENT_CONFIG_TYPE_RECTANGLE
    );
    return config;
}

static Clay_LayoutConfig layout(Clay_LayoutConfig config)
{
    Clay__AttachLayoutConfig(Clay__StoreLayoutConfig(config));
    return config;
}

static Clay_SizingAxis sizing_grow()
{
    return CLAY_SIZING_GROW();
}

static Clay_SizingAxis sizing_fixed(f32 n)
{
    return CLAY_SIZING_FIXED(n);
}

static Clay_ElementId id(StringView name)
{
    auto clay_name = Clay_String{
        .length = (i32)name.size(),
        .chars = name.data(),
    };
    auto id = Clay__HashString(clay_name, 0, 0);
    Clay__AttachId(id);
    return id;
}

static void text(StringView message, Clay_TextElementConfig config)
{
    auto s = Clay_String(message.size(), message.data());
    Clay__OpenTextElement(s, Clay__StoreTextElementConfig(config));
}

struct Element {
    Element() {
        Clay__OpenElement();
    }

    ~Element()
    {
        Clay__CloseElement();
    }

    template <typename F>
    Element& config(F callback)
    {
        callback();
        Clay__ElementPostConfiguration();
        return *this;
    }

    template <typename F>
    void body(F callback)
    {
        callback();
    }
};

}

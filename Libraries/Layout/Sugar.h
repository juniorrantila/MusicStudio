#pragma once
#include <Clay/Clay.h>
#include <Ty/StringView.h>
#include <Ty/SmallCapture.h>

namespace LayoutSugar {

struct ID {
    ID() = default;

    constexpr ID(ID const&) = default;
    constexpr ID(ID&&) = default;

    constexpr ID(StringView text)
        : text(text)
    {
    }

    constexpr ID(StringView text, u32 offset)
        : text(text)
        , offset(offset)
    {
    }

    constexpr ID& operator=(ID const&) = default;
    constexpr ID& operator=(ID&&) = default;

    constexpr explicit operator bool() const
    {
        return bool(text) && offset;
    }

    StringView text {};
    u32 offset {};
};

static inline Clay_RectangleElementConfig rectangle(Clay_RectangleElementConfig config)
{
    Clay__AttachElementConfig(Clay_ElementConfigUnion {
            .rectangleElementConfig = Clay__StoreRectangleElementConfig(config),
        },
        CLAY__ELEMENT_CONFIG_TYPE_RECTANGLE
    );
    return config;
}

static inline Clay_LayoutConfig layout(Clay_LayoutConfig config)
{
    Clay__AttachLayoutConfig(Clay__StoreLayoutConfig(config));
    return config;
}

static inline Clay_SizingAxis sizing_fixed(f32 n)
{
    return CLAY_SIZING_FIXED(n);
}

static inline Clay_SizingAxis sizing_grow()
{
    return CLAY_SIZING_GROW();
}

static inline Clay_SizingAxis sizing_grow(Clay_SizingMinMax minMax)
{
    return {
        .size = { .minMax = minMax },
        .type = CLAY__SIZING_TYPE_GROW
    };
}

static inline Clay_SizingAxis sizing_fit()
{
    return CLAY_SIZING_FIT();
}


static inline Clay_ElementId id(ID name)
{
    auto clay_name = Clay_String{
        .length = (i32)name.text.size(),
        .chars = name.text.data(),
    };
    auto id = Clay__HashString(clay_name, name.offset, 0);
    Clay__AttachId(id);
    return id;
}

static inline void text(StringView message, Clay_TextElementConfig config)
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


template <typename F>
void vstack(F callback)
{
    Element().config([]{
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = {},
            .childGap = {},
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            
        });
    }).body(callback);
}

template <typename F>
void vstack(ID name, F callback)
{
    Element().config([=]{
        id(name);
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = {},
            .childGap = {},
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            
        });
    }).body(callback);
}

struct StackConfig {
    ID id;
    Clay_Padding padding;
    u16 child_gap;
    Clay_ChildAlignment child_alignment;
    Clay_Color color;
};

template <typename F>
void vstack(StackConfig config, F callback)
{
    Element().config([=]{
        if (config.id) {
            id(config.id);
        }
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = config.padding,
            .childGap = config.child_gap,
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        });
        rectangle({
            .color = config.color,
            .cornerRadius = {},
        });
        CLAY_SCROLL({
            .horizontal = false,
            .vertical = true,
        });
    }).body(callback);
}

template <typename F>
void hstack(F callback)
{
    Element().config([]{
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = {},
            .childGap = {},
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            
        });
    }).body(callback);
}

template <typename F>
void hstack(StackConfig config, F callback)
{
    Element().config([=]{
        if (config.id) {
            id(config.id);
        }
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = config.padding,
            .childGap = config.child_gap,
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
        rectangle({
            .color = config.color,
            .cornerRadius = {},
        });
        CLAY_SCROLL({
            .horizontal = true,
            .vertical = false,
        });
    }).body(callback);
}

template <typename F>
void hstack(ID name, F callback)
{
    Element().config([=]{
        if (name) {
            id(name);
        }
        layout({
            .sizing = {
                .width = sizing_grow(),
                .height = sizing_grow(),
            },
            .padding = {},
            .childGap = {},
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
    }).body(callback);
}

struct BoxConfig {
    Clay_Padding padding;
    Clay_Sizing sizing;
    Clay_Color color;
    Clay_ChildAlignment child_alignment;
    void (*on_hover)(Clay_ElementId, Clay_PointerData, iptr);
    iptr on_hover_user;
};

template <typename F>
void box(ID name, SmallCapture<BoxConfig()> build_config, F callback)
{
    Element().config([&]{
        if (name) {
            id(name);
        }
        auto config = build_config();
        layout({
            .sizing = config.sizing,
            .padding = config.padding,
            .childGap = {},
            .childAlignment = config.child_alignment,
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
        rectangle({
            .color = config.color,
            .cornerRadius = {},
        });
        if (config.on_hover) {
            Clay_OnHover(config.on_hover, config.on_hover_user);
        }
    }).body(callback);
}

struct BoxConfig2 {
    ID id;
    Clay_Padding padding;
    Clay_Sizing sizing;
    Clay_Color color;
};

template <typename F>
void box(BoxConfig2 config, F callback)
{
    Element().config([=]{
        if (config.id) {
            id(config.id);
        }
        layout({
            .sizing = config.sizing,
            .padding = config.padding,
            .childGap = {},
            .childAlignment = {
                .x = CLAY_ALIGN_X_LEFT,
                .y = CLAY_ALIGN_Y_TOP,
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        });
        rectangle({
            .color = config.color,
            .cornerRadius = {},
        });
    }).body(callback);
}


}

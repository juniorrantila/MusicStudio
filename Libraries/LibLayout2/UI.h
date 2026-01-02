#pragma once
#include <LibTy/Base.h>
#include <LibCore/FSVolume.h>

typedef struct Layout Layout;
typedef struct LayoutColor LayoutColor;

typedef enum Align {
    Align_Start,
    Align_Center,
    Align_End,
} Align;

typedef enum Justify {
    Justify_Start,
    Justify_Center,
    Justify_End,
} Justify;

typedef enum LayoutDirection {
    LayoutDirection_Main,
    LayoutDirection_Cross,
} LayoutDirection;

typedef enum OverflowBehavior {
    OverflowBehavior_Grow,
    OverflowBehavior_Clip,
    OverflowBehavior_Scroll,
} OverflowBehavior;

C_API [[nodiscard]] bool box_begin(Layout*);
C_API void box_end(Layout*);
C_API void box_debug(Layout*, u32 id);

C_API void box_position_x(Layout*, f32);
C_API void box_position_y(Layout*, f32);
C_API void box_position(Layout*, f32 x, f32 y);
C_API void box_overflow_behavior(Layout*, OverflowBehavior);
C_API void box_overflow_x_behavior(Layout*, OverflowBehavior);
C_API void box_overflow_y_behavior(Layout*, OverflowBehavior);

C_API void box_id(Layout*, usize);
C_API void box_color(Layout*, LayoutColor);
C_API void box_color4(Layout*, f32, f32, f32, f32);

C_API void box_width(Layout*, f32);
C_API void box_height(Layout*, f32);

C_API void box_min_width(Layout*, f32);
C_API void box_min_height(Layout*, f32);

C_API void box_max_width(Layout*, f32);
C_API void box_max_height(Layout*, f32);

C_API void box_layout_direction(Layout*, LayoutDirection);
C_API void box_justify_children(Layout*, Justify);
C_API void box_align_children(Layout*, Align);
C_API void box_child_gap(Layout*, f32);

C_API void box_padding(Layout*, f32);
C_API void box_padding4(Layout*, f32 left, f32 top, f32 right, f32 bottom);

C_API void box_rounding(Layout*, f32);
C_API void box_rounding4(Layout*, f32 top_left, f32 top_right, f32 bottom_left, f32 bottom_right);

C_API void box_outline_size(Layout*, f32);
C_API void box_outline_size4(Layout*, f32 left, f32 top, f32 right, f32 bottom);

C_API void box_outline_color(Layout*, f32, f32, f32, f32);
C_API void box_outline_color4(Layout*,
    f32 left_r,     f32 left_g,     f32 left_b,     f32 left_a,
    f32 top_r,      f32 top_g,      f32 top_b,      f32 top_a,
    f32 right_r,    f32 right_g,    f32 right_b,    f32 right_a,
    f32 bottom_r,   f32 bottom_g,   f32 bottom_b,   f32 bottom_a
);

C_API bool box_mouse_down(Layout*);
C_API bool box_pressed(Layout*);
C_API u8   box_clicked(Layout*);
C_API bool box_hovered(Layout*);

C_API void layout_text(Layout*, FileID font, StringSlice text);
C_API void text_font_size(Layout*, f32);
C_API void text_letter_spacing(Layout*, f32);
C_API void text_line_height(Layout*, f32);
C_API void text_color(Layout*, f32, f32, f32, f32);
C_API void text_wrap(Layout*, bool);

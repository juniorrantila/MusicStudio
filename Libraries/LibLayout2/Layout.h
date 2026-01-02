#pragma once
#include "./UI.h"

#include <Basic/Types.h>
#include <Basic/Allocator.h>
#include <Basic/StringSlice.h>

#include <LibThread/MessageQueue.h>

#include <LibCore/FSVolume.h>

constexpr u32 layout_root_nodes_max = 16;
constexpr u32 layout_parent_stack_max = 32;
constexpr u32 layout_element_pool_max = 8192;
constexpr u32 layout_child_pool_max = 128;

typedef struct LayoutID { u64 hash; } LayoutID;
C_INLINE const LayoutID layout_id_null = { .hash = 0 };
C_INLINE LayoutID layout_id(u64 hash) { VERIFY(hash != 0); return (LayoutID){ .hash = hash }; }

typedef struct LayoutChildID LayoutChildID;
C_EXTERN const LayoutChildID layout_child_null;
typedef struct [[nodiscard]] LayoutChildID {
    u16 index;

#ifdef __cplusplus
        bool is_valid() const { return index != layout_child_null.index; }
#endif
} LayoutChildID;
C_INLINE const LayoutChildID layout_child_null = { .index = 0xFFFF };
C_INLINE LayoutChildID layout_child_id(u32 index) { VERIFY(index < layout_child_pool_max); return (LayoutChildID){.index = (u16)index}; }
C_INLINE bool layout_child_equal(LayoutChildID a, LayoutChildID b) { return a.index == b.index; }

typedef struct LayoutElementID LayoutElementID;
C_API const LayoutElementID layout_element_null;
typedef struct [[nodiscard]] LayoutElementID {
    u16 index;

#ifdef __cplusplus
    bool is_valid() const { return index != layout_element_null.index; }
#endif
} LayoutElementID;
C_INLINE const LayoutElementID layout_element_null = { .index = 0xFFFF };
C_INLINE LayoutElementID layout_element_id(u32 index) { VERIFY(index < layout_element_pool_max); return (LayoutElementID){.index = (u16)index}; }
C_INLINE bool layout_element_equal(LayoutElementID a, LayoutElementID b) { return a.index == b.index; }

typedef struct LayoutParentID LayoutParentID;
C_API const LayoutParentID layout_parent_null;
typedef struct [[nodiscard]] LayoutParentID {
    u8 index;

#ifdef __cplusplus
    bool is_valid() const { return index != layout_parent_null.index; }
#endif
} LayoutParentID;
C_INLINE const LayoutParentID layout_parent_null = { .index = 0xFF };
C_INLINE LayoutParentID layout_parent_id(u32 index) { VERIFY(index < layout_parent_stack_max); return (LayoutParentID){.index = (u8)index}; }
C_INLINE bool layout_parent_equal(LayoutParentID a, LayoutParentID b) { return a.index == b.index; }

typedef struct LayoutRootID LayoutRootID;
C_API const LayoutRootID layout_root_null;
typedef struct [[nodiscard]] LayoutRootID {
    u8 index;

#ifdef __cplusplus
    bool is_valid() const { return index != layout_root_null.index; }
#endif
} LayoutRootID;
C_INLINE const LayoutRootID layout_root_null = { .index = 0xFF };
C_INLINE LayoutRootID layout_root_id(u32 index) { VERIFY(index < layout_root_nodes_max); return (LayoutRootID){.index = (u8)index}; }
C_INLINE bool layout_root_equal(LayoutRootID a, LayoutRootID b) { return a.index == b.index; }

typedef struct LayoutChildPoolID LayoutChildPoolID;
C_API const LayoutChildPoolID layout_child_pool_null;
typedef struct [[nodiscard]] LayoutChildPoolID {
    u16 index;

#ifdef __cplusplus
    bool is_valid() const { return index != layout_child_pool_null.index; }
#endif
} LayoutChildPoolID;
C_INLINE const LayoutChildPoolID layout_child_pool_null = { .index = 0xFFFF };
C_INLINE LayoutChildPoolID layout_child_pool_id(u32 index) { VERIFY(index < layout_child_pool_max); return (LayoutChildPoolID){.index = (u16)index}; }
C_INLINE bool layout_child_pool_equal(LayoutChildPoolID a, LayoutChildPoolID b) { return a.index == b.index; }

typedef struct LayoutColor {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} LayoutColor;

typedef struct LayoutPoint {
    f32 x;
    f32 y;
} LayoutPoint;

typedef struct LayoutSize {
    f32 width;
    f32 height;
} LayoutSize;

typedef struct LayoutBox {
    LayoutPoint point;
    LayoutSize size;
} LayoutBox;

typedef struct LayoutRounding {
    f32 top_left;
    f32 top_right;
    f32 bottom_left;
    f32 bottom_right;
} LayoutRounding;

typedef struct LayoutPadding {
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
} LayoutPadding;

typedef struct LayoutShaderVertex {
    f32 x;
    f32 y;

    f32 r;
    f32 g;
    f32 b;
    f32 a;

    f32 u;
    f32 v;
} LayoutShaderVertex;

typedef struct LayoutElement LayoutElement;

typedef struct LayoutOutline {
    struct {
        f32 left;
        f32 top;
        f32 right;
        f32 bottom;
    } size;

    struct {
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        } left;
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        } top;
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        } right;
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        } bottom;
    } color;
} LayoutOutline;

typedef struct LayoutText {
    FileID font;
    StringSlice characters;
    f32 font_size;
    f32 letter_spacing;
    f32 line_height;

    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    } color;

    bool wrap;
} LayoutText;

typedef enum LayoutSizing : u8 {
    LayoutSizing_Fit,
    LayoutSizing_Fixed,
    LayoutSizing_Grow,
} LayoutSizing;

typedef struct LayoutElement {
    LayoutID id;

    LayoutElementID parent;

    u32 debug_id;

    struct {
        f32 x;
        f32 y;
    } position;
    LayoutColor color;
    struct {
        f32 x;
        f32 y;
    } scroll;

    struct {
        f32 width;
        f32 height;
    } size;
    struct {
        f32 width;
        f32 height;
    } min_size;
    struct {
        f32 width;
        f32 height;
    } max_size;
    struct {
        f32 x;
        f32 y;
    } layout_size;

    LayoutDirection layout_direction;
    OverflowBehavior overflow_x_behavior;
    OverflowBehavior overflow_y_behavior;

    Align child_align;
    Justify child_justify;
    f32 child_gap;

    struct {
        f32 left;
        f32 top;
        f32 right;
        f32 bottom;
    } padding;
    struct {
        f32 top_left;
        f32 top_right;
        f32 bottom_left;
        f32 bottom_right;
    } rounding;

    LayoutOutline outline;
    LayoutText text;

    struct {
        LayoutChildPoolID pool;
        u32 count;
    } children;

    LayoutSizing x_sizing;
    LayoutSizing y_sizing;
    u8 flex_factor_x;
    u8 flex_factor_y;
} LayoutElement;

typedef struct LayoutInputState {
    THMessageQueue* render_command_sink;

    f32 current_time;

    f32 frame_bounds_x;
    f32 frame_bounds_y;
    f32 pixel_ratio;

    f32 mouse_x;
    f32 mouse_y;
    i32 mouse_left_down;
    i32 mouse_right_down;

    f32 scroll_delta_x;
    f32 scroll_delta_y;
} LayoutInputState;

typedef struct LayoutPass {
    LayoutElementID items[layout_element_pool_max];
    u32 count;

#ifdef __cplusplus
    void push(LayoutElementID);
#endif
} LayoutPass;

typedef struct Layout {
    Logger* debug;

    LayoutBegin last_begin;
    LayoutBegin current_begin;

    LayoutID current_id;
    LayoutID active_id;
    LayoutID pressed_id;
    LayoutElementID current_element;

    f32 hover_duration;
    u64 click_count;

    struct {
        f32 x;
        f32 y;
        f32 time;
    } scroll_delta;

    struct {
        LayoutElementID items[layout_root_nodes_max];
        u32 count;
    } root_nodes;

    struct {
        LayoutElementID items[layout_parent_stack_max];
        u32 count;
    } parent_stack;

    struct {
        LayoutElement items[layout_element_pool_max];
        u32 count;
    } element_pool;

    struct {
        LayoutElementID items[layout_element_pool_max][layout_child_pool_max];
        u32 count;
    } child_pool;

    LayoutPass pass;

    u8 arena_buffer[32 * KiB];

#ifdef __cplusplus
    void begin(LayoutInputState);
    void end();

    LayoutParentID parent_push(LayoutElementID);
    LayoutElementID parent_peek();
    LayoutElementID parent_pop();

    LayoutRootID root_node_push(LayoutElementID);

    LayoutElementID element_push(LayoutElement);

    LayoutChildID child_push(LayoutElement* parent, LayoutElementID child);

    LayoutElement* resolve_element(LayoutElementID);
    LayoutElementID* resolve_child_pool(LayoutChildPoolID);
#endif
} Layout;

C_API void layout_init(Layout*, Logger* debug);

C_API void layout_begin(Layout*, LayoutInputState);
C_API void layout_end(Layout*);

C_API LayoutParentID layout_parent_push(Layout*, LayoutElementID);
C_API LayoutElementID layout_parent_pop(Layout*);
C_API LayoutElementID layout_parent_peek(Layout*);

C_API LayoutRootID layout_root_nodes_push(Layout*, LayoutElementID);

C_API LayoutElementID layout_element_push(Layout*, LayoutElement);
C_API LayoutChildID layout_child_push(Layout*, LayoutElement* parent, LayoutElementID child);

C_API LayoutElement layout_element(LayoutID id, LayoutElementID parent);
C_API LayoutElement* layout_resolve_element(Layout*, LayoutElementID);

C_API LayoutElementID* layout_resolve_child_pool(Layout*, LayoutChildPoolID);

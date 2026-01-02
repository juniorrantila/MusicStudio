#include "./Layout.h"

#include "./RenderCommand.h"
#include "./UI.h"

#include <Basic/Context.h>
#include <Basic/Verify.h>
#include <Basic/Defer.h>

static void layout_pass_depth_first(Layout*, LayoutElement*, LayoutPass*);
static void layout_pass_breadth_first(Layout* layout, LayoutElement* root_node, LayoutPass* layout_pass);

C_API void layout_init(Layout* layout, Logger* debug)
{
    memzero(layout);
    layout->debug = debug;
}

void Layout::begin(LayoutInputState begin) { return layout_begin(this, begin); }
C_API void layout_begin(Layout* layout, LayoutInputState begin)
{
    layout->last_input_state = layout->input_state;
    layout->input_state = begin;

    if (
        layout->last_input_state.frame_bounds_x != begin.frame_bounds_x
        || layout->last_input_state.frame_bounds_y != begin.frame_bounds_y
    ) {
        auto command = (LayoutRenderSetResolution){
            .width = begin.frame_bounds_x,
            .height = begin.frame_bounds_y,
        };
        if (!th_message_send(begin.render_command_sink, command).ok) errorf("could not post layout resolution to renderer");
    }

    if (!th_message_send(begin.render_command_sink, (LayoutRenderRectangle){
        .bounding_box = {
            .point = { 0, 0 },
            .size = { begin.frame_bounds_x, begin.frame_bounds_y },
        },
        .color = {
            .r = 0,
            .g = 0,
            .b = 0,
            .a = 1,
        },
        .debug_id = 0,
    }).ok) {
        if (layout->debug) layout->debug->error("could not post layout initial rectangle to renderer");
    }

    layout->root_nodes.count = 0;
    layout->parent_stack.count = 0;
    layout->element_pool.count = 0;
    layout->child_pool.count = 0;
    layout->current_element = layout_element_null;
    layout->current_id.hash = 0;
}

void Layout::end() { return layout_end(this); }
C_API void layout_end(Layout* l)
{
    if (!th_message_send(l->input_state.render_command_sink, LayoutRenderFlush{}).ok) {
        if (l->debug) l->debug->error("could not post flush command");
    }

}

LayoutElementID Layout::parent_peek() { return layout_parent_peek(this); }
C_API LayoutElementID layout_parent_peek(Layout* l)
{
    if (l->parent_stack.count <= 0)
        return layout_element_null;
    return l->parent_stack.items[l->parent_stack.count - 1];
}

LayoutElementID Layout::parent_pop() { return layout_parent_pop(this); }
C_API LayoutElementID layout_parent_pop(Layout* l)
{
    VERIFY(l->parent_stack.count >= 1);
    l->parent_stack.count -= 1;
    return l->parent_stack.items[l->parent_stack.count];
}


LayoutElement* Layout::resolve_element(LayoutElementID id) { return layout_resolve_element(this, id); }
C_API LayoutElement* layout_resolve_element(Layout* l, LayoutElementID id)
{
    if (layout_element_equal(id, layout_element_null)) return nullptr;
    if (id.index >= l->element_pool.count) return nullptr;
    return &l->element_pool.items[id.index];
}

LayoutChildID Layout::child_push(LayoutElement* parent, LayoutElementID child) { return layout_child_push(this, parent, child); }
C_API LayoutChildID layout_child_push(Layout* layout, LayoutElement* parent, LayoutElementID child)
{
    VERIFY(parent);

    VERIFY(!layout_element_equal(child, layout_element_null));
    VERIFY(parent->children.count < layout_element_pool_max);

    if (layout_child_pool_equal(parent->children.pool, layout_child_pool_null)) {
        VERIFY(layout->child_pool.count < layout_child_pool_max);
        parent->children.pool = layout_child_pool_id(layout->child_pool.count++);
    }

    VERIFY(parent->children.pool.index < layout->child_pool.count);
    u32 id = parent->children.count++;
    auto* pool = layout->resolve_child_pool(parent->children.pool);
    pool[id] = child;
    return layout_child_id(id);
}

LayoutRootID Layout::root_node_push(LayoutElementID element) { return layout_root_nodes_push(this, element); }
C_API LayoutRootID layout_root_nodes_push(Layout* layout, LayoutElementID element)
{
    if (layout->root_nodes.count + 1 >= ARRAY_SIZE(layout->root_nodes.items))
        return layout_root_null;
    auto id = layout_root_id(layout->root_nodes.count++);
    layout->root_nodes.items[id.index] = element;
    return id;
}

LayoutParentID Layout::parent_push(LayoutElementID element) { return layout_parent_push(this, element); }
C_API LayoutParentID layout_parent_push(Layout* layout, LayoutElementID element)
{
    if (layout->parent_stack.count + 1 >= ARRAY_SIZE(layout->parent_stack.items))
        return layout_parent_null;
    auto id = layout_parent_id(layout->parent_stack.count++);
    layout->parent_stack.items[id.index] = element;
    return id;
}

LayoutElementID Layout::element_push(LayoutElement element) { return layout_element_push(this, element); }
C_API LayoutElementID layout_element_push(Layout* layout, LayoutElement element)
{
    if (layout->element_pool.count + 1 >= ARRAY_SIZE(layout->element_pool.items))
        return layout_element_null;
    auto id = layout_element_id(layout->element_pool.count++);
    layout->element_pool.items[id.index] = element;
    return id;
}

LayoutElementID* Layout::resolve_child_pool(LayoutChildPoolID id) { return layout_resolve_child_pool(this, id); }
C_API LayoutElementID* layout_resolve_child_pool(Layout* l, LayoutChildPoolID id)
{
    if (layout_child_pool_equal(id, layout_child_pool_null)) return nullptr;
    if (id.index >= l->child_pool.count) return nullptr;
    VERIFY(id.index < layout_child_pool_max);
    return l->child_pool.items[id.index];
}

C_API LayoutElement layout_element(LayoutID id, LayoutElementID parent)
{
    return (LayoutElement){
        .id = id,
        .parent = parent,
        .debug_id = 0,
        .position = {
            .x = 0,
            .y = 0,
        },
        .color = {
            .r = 0,
            .g = 0,
            .b = 0,
            .a = 0,
        },
        .scroll = {
            .x = 0,
            .y = 0,
        },
        .size = {
            .width = 0,
            .height = 0,
        },
        .min_size = {
            .width = 0,
            .height = 0,
        },
        .max_size = {
            .width = 0,
            .height = 0,
        },
        .layout_size = {
            .x = 0,
            .y = 0,
        },
        .layout_direction = LayoutDirection_Main,
        .overflow_x_behavior = OverflowBehavior_Clip,
        .overflow_y_behavior = OverflowBehavior_Clip,

        .child_align = Align_Start,
        .child_justify = Justify_Start,
        .child_gap = 0,

        .padding = {
            .left = 0,
            .top = 0,
            .right = 0,
            .bottom = 0,
        },

        .rounding = {
            .top_left = 0,
            .top_right = 0,
            .bottom_left = 0,
            .bottom_right = 0,
        },

        .outline = {
            .size = {
                .left = 0,
                .top = 0,
                .right = 0,
                .bottom = 0,
            },
            .color = {
                .left = {
                    .r = 0,
                    .g = 0,
                    .b = 0,
                    .a = 0,
                },
                .top = {
                    .r = 0,
                    .g = 0,
                    .b = 0,
                    .a = 0,
                },
                .right = {
                    .r = 0,
                    .g = 0,
                    .b = 0,
                    .a = 0,
                },
                .bottom = {
                    .r = 0,
                    .g = 0,
                    .b = 0,
                    .a = 0,
                },
            },
        },
        .text = {
            .font = { 0 },
            .characters = {
                .items = nullptr,
                .count = 0,
            },
            .font_size = 0,
            .letter_spacing = 0,
            .line_height = 0,
            .color = {
                .r = 0,
                .g = 0,
                .b = 0,
                .a = 0,
            },
            .wrap = false,
        },
        .children = {
            .pool = layout_child_pool_null,
            .count = 0,
        },
        .x_sizing = LayoutSizing_Fit,
        .y_sizing = LayoutSizing_Fit,
        .flex_factor_x = 0,
        .flex_factor_y = 0,
    };
}

// 
// box_begin(1) -> root_push(id: 1)
//   box_min_width()
//
//   box_begin(2) -> child_push(parent: 1, child: 2)
//      box_min_height()
//      text()
//      text()
//   box_end(2) -> current_parent(1)
//
//   box_begin(3) -> child_push(parent: 1, child: 3)
//      box_min_height()
//      box_begin(4) -> child_push(parent: 3, child: 4)
//          box_begin(5) -> child_push(parent: 4, child: 5)
//              box_begin(6) -> child_push(parent: 5, child: 6)
//              box_end(6) -> current_parent(5)
//          box_end(5) -> current_parent(4)
//      box_end(4) -> current_parent(3)
//
//      box_begin(7) -> child_push(parent: 3, child: 7)
//          box_begin(8) -> child_push(parent: 7, child: 8)
//              box_begin(9) -> child_push(parent: 8, child: 9)
//              box_end(9) -> current_parent(8)
//          box_end(8) -> current_parent(7)
//      box_end(7) -> current_parent(3)
//   box_end(3) -> current_parent(1)
//
//   text()
// box_end() -> 0
//

static inline f32 max(f32 a, f32 b) { return a > b ? a : b; }

static void layout_fit_sizing_pass(Layout*, LayoutElement* root);
static void layout_grow_sizing_pass(Layout*, LayoutElement* root);
static void layout_position_pass(Layout*, LayoutElement* root);
static void layout_draw_pass(Layout*, LayoutElement* root);

static void grow_children(Layout* layout, LayoutElement* parent)
{
    auto* children = layout->resolve_child_pool(parent->children.pool);
    if (!children) return;

    f32 remaining_width = parent->layout_size.x;
    f32 remaining_height = parent->layout_size.y;

    f32 total_flex_factor_x = 0;
    f32 total_flex_factor_y = 0;
    bool grow_elements = false;
    for (u32 child_index = 0; child_index < parent->children.count; child_index++) {
        auto* child = layout->resolve_element(children[child_index]);
        if (verify(child != nullptr).failed)
            continue;
        remaining_width -= child->layout_size.x;
        if (child->x_sizing == LayoutSizing_Grow) {
            grow_elements = true;
            total_flex_factor_x += (f32)child->flex_factor_x;
        }
        remaining_height -= child->layout_size.y;
        if (child->y_sizing == LayoutSizing_Grow) {
            grow_elements = true;
            total_flex_factor_y += (f32)child->flex_factor_x;
        }
    }
    if (!grow_elements) return;

    remaining_width -= parent->padding.left + parent->padding.top;
    remaining_height -= parent->padding.top + parent->padding.bottom;
    remaining_width -= ((f32)(parent->children.count - 1)) * parent->child_gap;
    f32 flex_width = total_flex_factor_x == 0 ? 0 : remaining_width * (1.0f / total_flex_factor_x);
    f32 flex_height = total_flex_factor_y == 0 ? 0 : remaining_height * (1.0f / total_flex_factor_y);
    for (u32 child_index = 0; child_index < parent->children.count; child_index++) {
        auto* child = layout->resolve_element(children[child_index]);
        if (verify(child != nullptr).failed)
            continue;
        if (child->x_sizing == LayoutSizing_Grow) {
            child->layout_size.x += flex_width * (f32)child->flex_factor_x;
        }
        if (child->y_sizing == LayoutSizing_Grow) {
            child->layout_size.y += flex_height * (f32)child->flex_factor_y;
        }
    }
}


typedef struct LayoutElementQueue {
    u32 head;
    u32 tail;
    LayoutElementID items[layout_element_pool_max];
    
#ifdef __cplusplus
    void push(LayoutElementID);
    LayoutElementID pop();
    bool is_empty() const;
#endif
} LayoutElementQueue;
static void layout_element_queue_init(LayoutElementQueue* queue)
{
    queue->head = 0;
    queue->tail = 0;
}

static bool layout_element_queue_is_empty(LayoutElementQueue const* queue) { return queue->head == queue->tail; }
bool LayoutElementQueue::is_empty() const { return layout_element_queue_is_empty(this); }

static void layout_element_queue_push(LayoutElementQueue* queue, LayoutElementID element)
{
    queue->items[queue->head % ARRAY_SIZE(queue->items)] = element;
    queue->head += 1; 
}
void LayoutElementQueue::push(LayoutElementID id) { return layout_element_queue_push(this, id); }

static LayoutElementID layout_element_queue_pop(LayoutElementQueue* queue)
{
    VERIFY(queue->tail < queue->head);
    LayoutElementID result = queue->items[queue->tail % ARRAY_SIZE(queue->items)];
    queue->tail += 1;
    return result;
}
LayoutElementID LayoutElementQueue::pop() { return layout_element_queue_pop(this); }

static void layout_pass_init(LayoutPass* pass) { pass->count = 0; }
static void layout_pass_push(LayoutPass* pass, LayoutElementID id)
{
    VERIFY(pass->count + 1 < layout_element_pool_max);
    pass->items[pass->count++] = id;
}
void LayoutPass::push(LayoutElementID id) { layout_pass_push(this, id); }

// Depth first:
//
//         r
//        /|\
//       / o \
//      / /|\ \
//     q f g h p
//    /|\     /|\
//   l m n   i j k
//  / /|\ \
// a b c d e
//
[[maybe_unused]]
static void layout_pass_depth_first(Layout*, LayoutElement*, LayoutPass*)
{
    UNIMPLEMENTED();
}

// Breadth first:
//
//         a
//        /|\
//       / c \
//      / /|\ \
//     b h i j d
//    /|\     /|\
//   e f g   k l m
//  / /|\ \
// n o p q r
static void layout_pass_breadth_first(Layout* layout, LayoutElement* root_node, LayoutPass* layout_pass)
{
    VERIFY(layout != nullptr);
    VERIFY(root_node != nullptr);
    VERIFY(layout_pass != nullptr);

    layout_pass_init(layout_pass);

    LayoutElementQueue queue = (LayoutElementQueue){};
    layout_element_queue_init(&queue);

    auto* root_children = layout->resolve_child_pool(root_node->children.pool);
    u32 root_children_count = root_node->children.count;
    for (u32 child_index = 0; child_index < root_children_count; child_index += 1)
        queue.push(root_children[child_index]);

    while (!queue.is_empty()) {
        auto node = queue.pop();
        auto* element = layout->resolve_element(node);
        layout_pass->push(node);
        auto* children = layout->resolve_child_pool(element->children.pool);
        u32 children_count = element->children.count;
        for (u32 child_index = 0; child_index < children_count; child_index += 1)
            queue.push(children[child_index]);
    }
}

static void layout_fit_sizing_pass(Layout* layout, LayoutElement* root_node)
{
    auto* children = layout->resolve_child_pool(root_node->children.pool);
    if (!children) return;

    layout_pass_breadth_first(layout, root_node, &layout->pass);

    for (u32 i = 0; i < layout->pass.count; i++) {
        auto* item = layout->resolve_element(layout->pass.items[i]);
        VERIFY(item != nullptr);
    }
}

static void layout_grow_sizing_pass(Layout* layout, LayoutElement* root_node)
{
    auto* children = layout->resolve_child_pool(root_node->children.pool);
    if (!children) return;

    layout_pass_breadth_first(layout, root_node, &layout->pass);

    for (u32 i = 0; i < layout->pass.count; i++)
        grow_children(layout, layout->resolve_element(layout->pass.items[i]));
}

static void layout_position_pass(Layout*, LayoutElement*)
{
}

static void layout_draw_pass(Layout* layout, LayoutElement* root_node)
{
    auto recurse = [=](auto recurse, LayoutElement* element, LayoutElement* parent, f32 rel_x, f32 rel_y) {
        auto& position = element->position;
        auto& size = element->size;
        auto& max_size = element->max_size;
        auto color = element->color;
        position.x += rel_x;
        position.y += rel_y;

        if (parent) {
            if (max_size.width == 0 || parent->size.width < max_size.width)
                max_size.width = parent->size.width;
            if (max_size.height == 0 || parent->size.height < max_size.height)
                max_size.height = parent->size.height;
        }
        if (size.width == 0) size.width = max_size.width;
        if (size.height == 0) size.height = max_size.height;
        if (max_size.width < size.width)
            size.width = max_size.width;
        if (max_size.height < size.height)
            size.height = max_size.height;

        auto command = (LayoutRenderRectangle){
            .bounding_box = {
                .point = {
                    .x = position.x,
                    .y = position.y,
                },
                .size = {
                    .width = size.width,
                    .height = size.height,
                },
            },
            .color = {
                .r = color.r,
                .g = color.g,
                .b = color.b,
                .a = color.a,
            },
            .debug_id = element->debug_id,
        };
        if (layout->input_state.mouse_x >= position.x && layout->input_state.mouse_x <= position.x + size.width) {
            if (layout->input_state.mouse_y >= position.y && layout->input_state.mouse_y <= position.y + size.height) {
                layout->active_id = element->id;
                layout->mouse_down_id = layout->input_state.mouse_left_down ? layout->active_id : layout_id_null;
                if (layout->pressed_id.hash == layout->active_id.hash) {
                    layout->pressed_id = layout_id_null;
                } else {
                    if (!layout->last_input_state.mouse_left_down) {
                        layout->pressed_id = layout->input_state.mouse_left_down ? layout->active_id : layout_id_null;
                    }
                }
            }
        }

        if (!th_message_send(layout->input_state.render_command_sink, command).ok) {
            if (layout->debug) layout->debug->error("could not push layout command");
            return;
        }

        auto* child_pool = layout_resolve_child_pool(layout, element->children.pool);
        for (u32 child = 0; child < element->children.count; child++) {
            VERIFY(child_pool != nullptr);
            auto* child_element = layout_resolve_element(layout, child_pool[child]);
            recurse(recurse, child_element, element, rel_x, rel_y);
            if (element->layout_direction == LayoutDirection_Main) {
                rel_y += child_element->size.height;
                rel_y += element->child_gap;
            } else {
                rel_x += child_element->size.width;
                rel_x += element->child_gap;
            }
        }
    };
    recurse(recurse, root_node, nullptr, 0, 0);

    // layout_pass_breadth_first(layout, root_node, &layout->pass);
    // for (u32 i = 0; i < layout->pass.count; i++) {
    //     auto* item = layout->resolve_element(layout->pass.items[i]);
    //     auto command = (LayoutRenderRectangle){
    //         .bounding_box = {
    //             .point = {
    //                 .x = item->position.x,
    //                 .y = item->position.y,
    //             },
    //             .size = {
    //                 .width = item->layout_size.x,
    //                 .height = item->layout_size.y,
    //             },
    //         },
    //         .color = {
    //             .r = item->color.r,
    //             .g = item->color.g,
    //             .b = item->color.b,
    //             .a = item->color.a,
    //         },
    //     };
    //     if (!layout->current_begin.render_sink->writer()->post(command).ok) {
    //         if (layout->debug) layout->debug->error("could not post render command");
    //     }
    // }
}

C_API [[nodiscard]] bool box_begin(Layout* layout)
{
    LayoutID id = layout_id(++layout->current_id.hash);
    auto parent = layout->parent_peek();

    auto element = layout->element_push(layout_element(id, parent));
    if (!element.is_valid()) return false;
    layout->current_element = element;
    if (parent.is_valid()) {
        auto child = layout->child_push(layout->resolve_element(parent), element);
        if (!child.is_valid()) return false;
    } else {
        auto node = layout->root_node_push(element);
        if (!node.is_valid()) return false;
        auto* e = layout->resolve_element(element);
        VERIFY(e != nullptr);
        e->size.width = layout->input_state.frame_bounds_x;
        e->size.height = layout->input_state.frame_bounds_y;
        e->max_size.width = layout->input_state.frame_bounds_x;
        e->max_size.height = layout->input_state.frame_bounds_y;
    }
    auto node = layout->parent_push(element);
    if (!node.is_valid()) return false;
    return true;
}

C_API void box_end(Layout* layout)
{
    auto* current = layout->resolve_element(layout->current_element);
    VERIFY(current != nullptr);
    defer [=] { layout->current_element = current->parent; };
    VERIFY(layout_parent_pop(layout).is_valid());

    auto* parent = layout->resolve_element(current->parent);
    if (!parent) {
        layout_fit_sizing_pass(layout, current);
        layout_grow_sizing_pass(layout, current);
        layout_position_pass(layout, current);
        layout_draw_pass(layout, current);
        return;
    };

    auto padding = current->padding;
    current->layout_size.x += padding.left + padding.right;
    current->layout_size.y += padding.top + padding.bottom;
    f32 gap = ((f32)(parent->children.count - 1)) * parent->child_gap;

    switch (parent->layout_direction)
    case LayoutDirection_Main: {
        if (parent->x_sizing == LayoutSizing_Fit) {
            current->layout_size.x += gap;
            parent->layout_size.x += current->layout_size.x;
        }
        if (parent->y_sizing == LayoutSizing_Fit) {
            parent->layout_size.y = max(parent->layout_size.y, current->layout_size.y);
        }
        break;
    case LayoutDirection_Cross:
        if (parent->y_sizing == LayoutSizing_Fit) {
            current->layout_size.y += gap;
            parent->layout_size.y += current->layout_size.y;
        }
        if (parent->x_sizing == LayoutSizing_Fit) {
            parent->layout_size.x = max(parent->layout_size.x, current->layout_size.x);
        }
        break;
    }
}

C_API void box_debug(Layout* l, u32 id)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->debug_id = id;
}

C_API void box_position_x(Layout* l, f32 x)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->position.x = x;
}

C_API void box_position_y(Layout* l, f32 y) 
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->position.y = y;
}

C_API void box_position(Layout* l, f32 x, f32 y)
{
    box_position_x(l, x);
    box_position_y(l, y);
}

C_API void box_overflow_behavior(Layout* l, OverflowBehavior behavior)
{
    box_overflow_x_behavior(l, behavior);
    box_overflow_y_behavior(l, behavior);
}

C_API void box_overflow_x_behavior(Layout* l, OverflowBehavior behavior)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->overflow_x_behavior = behavior;
}

C_API void box_overflow_y_behavior(Layout* l, OverflowBehavior behavior)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->overflow_y_behavior = behavior;
}

C_API void box_id(Layout* l, usize id)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->id = layout_id(id); // FIXME: Use hashing.
}

C_API void box_color(Layout* l, LayoutColor color)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->color = color;
}

C_API void box_color4(Layout* l, f32 r, f32 g, f32 b, f32 a)
{
    box_color(l, (LayoutColor){
        .r = r,
        .g = g,
        .b = b,
        .a = a,
    });
}

C_API void box_width(Layout* l, f32 width)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->size.width = width;
}

C_API void box_height(Layout* l, f32 height)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->size.height = height;
}

C_API void box_min_width(Layout* l, f32 width)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->min_size.width = width;
}

C_API void box_min_height(Layout* l, f32 height)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->min_size.height = height;
}

C_API void box_max_width(Layout* l, f32 width)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->max_size.width = width;
}

C_API void box_max_height(Layout* l, f32 height)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->max_size.height = height;
}

C_API void box_layout_direction(Layout* l, LayoutDirection direction)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->layout_direction = direction;
}

C_API void box_justify_children(Layout* l, Justify justify)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->child_justify = justify;
}

C_API void box_align_children(Layout* l, Align align)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->child_align = align;
}

C_API void box_child_gap(Layout* l, f32 gap)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->child_gap = gap;
}

C_API void box_padding(Layout* l, f32 padding)
{
    box_padding4(l, padding, padding, padding, padding);
}

C_API void box_padding4(Layout* l, f32 left, f32 top, f32 right, f32 bottom)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->padding = {
        .left = left,
        .top = top,
        .right = right,
        .bottom = bottom,
    };
}

C_API void box_rounding(Layout* l, f32 rounding)
{
    box_rounding4(l, rounding, rounding, rounding, rounding);
}

C_API void box_rounding4(Layout* l, f32 top_left, f32 top_right, f32 bottom_left, f32 bottom_right)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->rounding = {
        .top_left = top_left,
        .top_right = top_right,
        .bottom_left = bottom_left,
        .bottom_right = bottom_right,
    };
}

C_API void box_outline_size(Layout* l, f32 size)
{
    box_outline_size4(l, size, size, size, size);
}

C_API void box_outline_size4(Layout* l, f32 left, f32 top, f32 right, f32 bottom)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->outline.size = {
        .left = left,
        .top = top,
        .right = right,
        .bottom = bottom,
    };
}

C_API void box_outline_color(Layout* l, f32 r, f32 g, f32 b, f32 a)
{
    box_outline_color4(l,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a,
        r, g, b, a
    );
}

C_API void box_outline_color4(Layout* l,
    f32 left_r,     f32 left_g,     f32 left_b,     f32 left_a,
    f32 top_r,      f32 top_g,      f32 top_b,      f32 top_a,
    f32 right_r,    f32 right_g,    f32 right_b,    f32 right_a,
    f32 bottom_r,   f32 bottom_g,   f32 bottom_b,   f32 bottom_a
)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->outline.color = {
        .left = {
            .r = left_r,
            .g = left_g,
            .b = left_b,
            .a = left_a,
        },
        .top = {
            .r = top_r,
            .g = top_g,
            .b = top_b,
            .a = top_a,
        },
        .right = {
            .r = right_r,
            .g = right_g,
            .b = right_b,
            .a = right_a,
        },
        .bottom = {
            .r = bottom_r,
            .g = bottom_g,
            .b = bottom_b,
            .a = bottom_a,
        },
    };
}

C_API bool box_mouse_down(Layout* l)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    return l->mouse_down_id.hash == current->id.hash;
}

C_API bool box_pressed(Layout* l)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    return l->pressed_id.hash == current->id.hash;
}

C_API u8 box_clicked(Layout* l)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    if (l->pressed_id.hash == current->id.hash) {
        return l->click_count;
    }
    return 0;
}

C_API bool box_hovered(Layout* l)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    return l->active_id.hash == current->id.hash;
}

C_API void layout_text(Layout* l, FileID font, StringSlice text)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.font = font;
    current->text.characters = text; // FIXME: Use rope string.
}

C_API void text_font_size(Layout* l, f32 size)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.font_size = size;
}

C_API void text_letter_spacing(Layout* l, f32 spacing)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.letter_spacing = spacing;
}

C_API void text_line_height(Layout* l, f32 line_height)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.line_height = line_height;
}

C_API void text_color(Layout* l, f32 r, f32 g, f32 b, f32 a)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.color = {
        .r = r,
        .g = g,
        .b = b,
        .a = a,
    };
}

C_API void text_wrap(Layout* l, bool wrap)
{
    auto* current = layout_resolve_element(l, l->current_element);
    VERIFY(current);
    current->text.wrap = wrap;
}

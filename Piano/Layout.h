#pragma once

typedef struct {
} ConstrainFill;

typedef struct {
} ConstrainFit;

typedef struct {
    float min;
    float max;
} ConstrainMinMax;

typedef struct {
    float value;
} ConstrainMin;

typedef struct {
    float value;
} ConstrainMax;

typedef struct {
    union {
        ConstrainFill fill;
        ConstrainFit fit;
        ConstrainMinMax min_max;
        ConstrainMin min;
        ConstrainMax max;
    };
    enum {
        SIZE_CONSTRAINT_FILL = 0,
        SIZE_CONSTRAINT_FIT,
        SIZE_CONSTRAINT_MIN_MAX,
        SIZE_CONSTRAINT_MIN,
        SIZE_CONSTRAINT_MAX,
    } tag;
} SizeConstraint;

typedef struct {
    SizeConstraint x;
    SizeConstraint y;
} Sizing;

static inline SizeConstraint constrain_fill(void)
{
    return (SizeConstraint){
        .fill = {},
        .tag = SIZE_CONSTRAINT_FILL,
    };
}

static inline SizeConstraint constrain_fit(void)
{
    return (SizeConstraint){
        .fit = {},
        .tag = SIZE_CONSTRAINT_FIT,
    };
}

static inline SizeConstraint constrain_min_max(float min, float max)
{
    return (SizeConstraint){
        .min_max = {
            .min = min,
            .max = max,
        },
        .tag = SIZE_CONSTRAINT_MIN_MAX,
    };
}

static inline SizeConstraint constrain_absolute(float value)
{
    return constrain_min_max(value, value);
}

static inline SizeConstraint constrain_min(float value)
{
    return (SizeConstraint){
        .min = {
            .value = value,
        },
        .tag = SIZE_CONSTRAINT_MIN,
    };
}

static inline SizeConstraint constrain_max(float value)
{
    return (SizeConstraint){
        .max = {
            .value = value,
        },
        .tag = SIZE_CONSTRAINT_MAX,
    };
}

static inline Sizing sizing(Sizing sizing)
{
    return sizing;
}

static inline Sizing sizing_fill(void)
{
    return sizing((Sizing){
        .x = constrain_fill(),
        .y = constrain_fill(),
    });
}

static inline Sizing sizing_fit(void)
{
    return sizing((Sizing){
        .x = constrain_fit(),
        .y = constrain_fit(),
    });
}

typedef struct {
    ConstrainMinMax x;
    ConstrainMinMax y;
} SizingMinMax;

static inline Sizing sizing_min_max(SizingMinMax minMax)
{
    return (Sizing){
        .x = constrain_min_max(minMax.x.min, minMax.x.max),
        .y = constrain_min_max(minMax.y.min, minMax.y.max),
    };
}

typedef struct {
    float x;
    float y;
} SizingAbsolute;

static inline Sizing sizing_absolute(SizingAbsolute absolute)
{
    return (Sizing){
        .x = constrain_absolute(absolute.x),
        .y = constrain_absolute(absolute.y),
    };
}

typedef struct {
    float x;
    float y;
} SizingMin;

static inline Sizing sizing_min(SizingMin min)
{
    return (Sizing){
        .x = constrain_min(min.x),
        .y = constrain_min(min.y),
    };
}

typedef struct {
    float x;
    float y;
} SizingMax;

static inline Sizing sizing_max(SizingMax max)
{
    return (Sizing){
        .x = constrain_max(max.x),
        .y = constrain_max(max.y),
    };
}

typedef struct {
    float top_left;
    float top_right;
    float bottom_left;
    float bottom_right;
} BorderRadius;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

typedef enum {
    ALIGN_START,
    ALIGN_CENTER,
    ALIGN_END,
} Align;

typedef struct {
    Align main;
    Align cross;
} Alignment;

typedef struct {
    float value;
} SpaceGap;

typedef enum {
    SPACING_BETWEEN,
    SPACENG_AROUND,
} Spacing;

typedef enum {
    LAYOUT_ROW,
    LAYOUT_COLUMN,
} LayoutDirection;

typedef enum {
    OVERFLOW_CLIP,
    OVERFLOW_SCROLL,
} Overflow;

typedef struct {
    Overflow x;
    Overflow y;
} OverflowBehavior;

typedef enum {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_MIDDLE_LEFT,
    ANCHOR_MIDDLE_CENTER,
    ANCHOR_MIDDLE_RIGHT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT,
} Anchor;

typedef struct {
    char const* debug_label;

    LayoutDirection layout_direction;
    Sizing sizing;
    Alignment child_alignment;
    Spacing child_spacing;
    float child_gap;
    OverflowBehavior overflow;
    BorderRadius border_radius;
    Color fill_color;
    Color outline_color;
    float outline_smoothing;

    Anchor anchor;
    float rotation;
} Box;

Box* box(Box);
void box_end(Box*);

typedef struct {
} Interactable;

Interactable* interactable(Box* box, int id);
int interact_up(Interactable const*);
int interact_down(Interactable const*);
float interact_held(Interactable const*);
float interact_over(Interactable const*);

typedef struct {
    char const* text;
    Box* box;
    Interactable* interactable;
} Button;

typedef struct {
    char const* characters;
} Text;

void text(char const* characters);

Button* make_button(Button);

static inline Button* button(int id, char const* initial_text, Box initial_box)
{
    Box* b = box(initial_box);
    Interactable* i = interactable(b, id);
    Button* state = make_button((Button){
        .text = initial_text,
        .box = b,
        .interactable = i,
    });
    text(state->text);
    box_end(b);
    return state;
}

static inline int button_up(Button const* button)
{
    return interact_up(button->interactable);
}

static inline int button_down(Button const* button)
{
    return interact_down(button->interactable);
}

static inline float button_held(Button const* button)
{
    return interact_held(button->interactable);
}

static inline float button_over(Button const* button)
{
    return interact_over(button->interactable);
}

typedef enum {
    EXPLORER_TAB_FILES,
    EXPLORER_TAB_PROJECT,
    EXPLORER_TAB_PLUGINS,
} ExplorerTab;

typedef struct {
    char const* status;
} StatusBar;

void status_bar_info(StatusBar*, char const* fmt, ...);

typedef struct {
    char const** items;
    int count;
} FileExplorer;

typedef struct {
    StatusBar* status;

    ExplorerTab tab;

    FileExplorer* files;
} Explorer;

float animate(float t, float duration, float current, float to);

static inline void explorer(Explorer* explorer)
{
    Box* explorer_box = box((Box){
        .layout_direction = LAYOUT_ROW,
        .sizing = {
            .x = constrain_absolute(120),
            .y = constrain_fill(),
        }
    });

    {
        Box* tabs_box = box((Box){
            .layout_direction = LAYOUT_COLUMN,
            .sizing = {
                .x = constrain_fill(),
                .y = constrain_fit(),
            }
        });
        Button* files = button(0, "Files", (Box){
            .layout_direction = LAYOUT_COLUMN,
            .child_alignment = {
                .main = ALIGN_CENTER,
                .cross = ALIGN_CENTER,
            }
        });
        if (button_down(files)) {
            explorer->tab = EXPLORER_TAB_FILES;
        }

        Button* project = button(1, "Project", (Box){
            .layout_direction = LAYOUT_COLUMN,
            .child_alignment = {
                .main = ALIGN_CENTER,
                .cross = ALIGN_CENTER,
            }
        });
        if (button_down(project)) {
            explorer->tab = EXPLORER_TAB_PROJECT;
        }

        Button* plugins = button(2, "Plugins", (Box){
            .layout_direction = LAYOUT_COLUMN,
            .child_alignment = {
                .main = ALIGN_CENTER,
                .cross = ALIGN_CENTER,
            }
        });
        if (button_down(plugins)) {
            explorer->tab = EXPLORER_TAB_PLUGINS;
        }

        box_end(tabs_box);
    }

    // FIXME: Assuming files
    {
        Box* files_box = box((Box){
            .layout_direction = LAYOUT_ROW,
            .sizing = sizing_fill(),
        });

        FileExplorer* files = explorer->files;
        for (int i = 0; i < files->count; i++) {
            char const* file_name = files->items[i];
            Button* item = button(i, file_name, (Box){
                .layout_direction = LAYOUT_COLUMN,
                .child_alignment = {
                    .main = ALIGN_START,
                    .cross = ALIGN_CENTER,
                }
            });
            float over_duration = button_over(item);
            if (over_duration > 0.0f) {
                status_bar_info(explorer->status, "open %s", file_name);
                Color* current = &item->box->fill_color;
                Color initial = item->box->fill_color;
                current->r = animate(over_duration, 0.1f, current->r, initial.r);
                current->g = animate(over_duration, 0.1f, current->g, initial.g);
                current->b = animate(over_duration, 0.1f, current->b, initial.b);
                current->a = animate(over_duration, 0.1f, current->a, initial.a);
            } 
            if (over_duration == 0.0f) {
                status_bar_info(explorer->status, 0);
            }
            if (over_duration < 0.0f) {
                Color* current = &item->box->fill_color;
                Color initial = item->box->fill_color;
                current->r = animate(-over_duration, 0.1f, current->r, initial.r);
                current->g = animate(-over_duration, 0.1f, current->g, initial.g);
                current->b = animate(-over_duration, 0.1f, current->b, initial.b);
                current->a = animate(-over_duration, 0.1f, current->a, initial.a);
            }
        }

        box_end(files_box);
    }

    box_end(explorer_box);
}

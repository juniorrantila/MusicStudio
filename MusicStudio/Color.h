#pragma once
#include <Clay/Clay.h>
#include <LibRexim/LA.h>

static inline Vec2f zero  = { 0.0, 0.0 };
static inline Vec4f cyan = { .r = 0.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
static inline Vec4f magenta = { .r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
static inline Vec4f yellow = { .r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
static inline Vec4f white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
static inline Vec4f red = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
static inline Vec4f green = { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
static inline Vec4f blue = { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
static inline Vec4f border_color = hex_to_vec4f(0x2C3337FF);
static inline Vec4f outline_color = hex_to_vec4f(0x4B5255FF);
static inline Vec4f background_color = hex_to_vec4f(0x181F23FF);
static inline Vec4f button_color = hex_to_vec4f(0x99A89FFF);
static inline Vec4f gray_background = hex_to_vec4f(0x646A71FF);
static inline Vec4f gray = hex_to_vec4f(0xAAAAAAAA);
static inline Vec4f black = hex_to_vec4f(0x000000FF);
static inline Vec4f toolbar_color = hex_to_vec4f(0x5B6265FF);

static inline Clay_Color COLOR_BUTTON = { 0x99, 0xA8, 0x9F, 0xFF };
static inline Clay_Color COLOR_GRAY_BACKGROUND = { 0x64, 0x6A, 0x71, 0xFF};
static inline Clay_Color COLOR_OUTLINE = { 0x4B, 0x52, 0x55, 0xFF };
static inline Clay_Color COLOR_BORDER = { 0x2C, 0x33, 0x37, 0xFF };
static inline Clay_Color COLOR_BACKGROUND = { 0x18, 0x1f, 0x23, 0xff };
static inline Clay_Color COLOR_BLACK = { 0x00, 0x00, 0x00, 0xff };
static inline Clay_Color COLOR_ORANGE = {225, 138, 50, 255};
static inline Clay_Color COLOR_BLUE = {111, 173, 162, 255};
static inline Clay_Color COLOR_LIGHT = {224, 215, 210, 255};
static inline Clay_Color COLOR_WHITE = {255, 255, 255, 255};
static inline Clay_Color COLOR_TOOLBAR = {0x5B, 0x62, 0x65, 0xFF};

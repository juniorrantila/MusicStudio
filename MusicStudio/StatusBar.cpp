#include "./StatusBar.h"
#include "./Style.h"
#include <UI/UI.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

StatusBar& StatusBar::the()
{
    static auto status_bar = StatusBar();
    return status_bar;
}

void StatusBar::set_text(StatusKind kind, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char* dest = nullptr;
    vasprintf(&dest, fmt, args);
    m_text.clear();
    MUST(m_text.write(StringView::from_c_string(dest)));
    free(dest);

    m_kind = kind;

    va_end(args);
}

void StatusBar::render(UI::UI& ui, EventLoop&, Vec4f box)
{
    ui.outline_rect({
        .box = box,
        .outline_size = Style::the().border_size(),
        .fill_color = Style::the().status_bar_color(),
        .top_color = Style::the().status_bar_border_color(),
    });

    auto color = Style::the().text_info_color();
    if (StatusBar::the().text_kind() == StatusKind::Error) {
        color = Style::the().text_error_color();
    }

    auto font_size = Style::the().status_bar_font_size();
    auto indent = Style::the().file_browser_indent_width();
    MUST(ui.set_font_size(font_size));
    ui.text(
        box.start_point() + vec2f(indent, box.height / 2.0f - font_size.y / 2.0f) + vec2fs(2.0f),
        StatusBar::the().text(),
        color
    );
}

void flash_error(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char* dest = nullptr;
    vasprintf(&dest, fmt, args);
    fprintf(stderr, "ERROR: %s\n", dest);

    StatusBar::the().set_text(StatusKind::Error, "ERROR: %s", dest);
    free(dest);

    va_end(args);
}

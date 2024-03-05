#pragma once
#include "./Forward.h"
#include "./Widget.h"

#include <UI/Forward.h>

#include <Ty/Base.h>
#include <Ty/StringView.h>
#include <Ty/StringBuffer.h>

#include <Rexim/LA.h>

enum class StatusKind {
    Info,
    Error,
};

struct StatusBar : Widget {
    StatusBar(StatusBar&&) = delete;
    StatusBar& operator=(StatusBar&&) = delete;

    StatusBar(StatusBar const&) = delete;
    StatusBar& operator=(StatusBar const&) = delete;

    static StatusBar& the();
    
    [[gnu::format(printf, 3, 4)]]
    void set_text(StatusKind, c_string, ...);

    StringView text() const { return m_text.view(); }
    StatusKind text_kind() const { return m_kind; }

    void render(UI::UI& ui, EventLoop&, Vec4f box) override;

private:
    constexpr StatusBar() = default;

    StringBuffer m_text {};
    StatusKind m_kind { StatusKind::Info };
};

[[gnu::format(printf, 1, 2)]] void flash_error(c_string fmt, ...);

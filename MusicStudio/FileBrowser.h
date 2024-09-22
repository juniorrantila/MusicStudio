#pragma once
#include "./Forward.h"
#include "./Widget.h"

#include <Ty/Forward.h>
#include <UI/Forward.h>

#include <Rexim/File.h>
#include <Rexim/LA.h>
#include <Ty/Signal.h>
#include <Ty/StringBuffer.h>

struct FileBrowser : Widget {
    static ErrorOr<FileBrowser> create(StringView path);
    virtual ~FileBrowser() = default;

    ErrorOr<void> open_dir(StringView path);
    ErrorOr<void> change_dir();
    Optional<StringView> current_file();

    void render(UI::UI&, EventLoop& event_loop, Vec4f box) override;

private:
    constexpr FileBrowser() = default;

    Files m_files {};
    usize m_cursor { 0 };
    String_Builder m_dir_path {};
    String_Builder m_file_path {};
    StringView m_hovered_file {};
};

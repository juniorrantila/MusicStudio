#pragma once
#include "./Forward.h"

#include <Ty/Forward.h>
#include <UI/Forward.h>
#include <UIView/Forward.h>

#include <Rexim/File.h>
#include <Rexim/LA.h>
#include <Ty/Signal.h>
#include <Ty/StringBuffer.h>

struct FileBrowser {
    static ErrorOr<FileBrowser> create(StringView path);

    ErrorOr<void> open_dir(StringView path);
    ErrorOr<void> change_dir();
    Optional<StringView> current_file();

    UIView::ViewBase* view();
    operator UIView::ViewBase*() { return view(); }

    Signal<StringView> selected_file {};

private:
    constexpr FileBrowser() = default;

    Files m_files {};
    usize m_cursor { 0 };
    String_Builder m_dir_path {};
    String_Builder m_file_path {};
    StringView m_hovered_file {};
};
